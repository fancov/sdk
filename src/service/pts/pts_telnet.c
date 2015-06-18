#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include system header file */
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

/* include the dos header files */
#include <dos.h>
#include <pt/dos_sqlite3.h>
#include <pt/pt.h>
#include <pt/pts.h>
/* include provate header file */
#include "pts_telnet.h"
#include "../telnetd/telnetd.h"
#include "pts_msg.h"

#define PTS_TELNET_HTLP "Usage:\r\n\
\r\tlist [Internet IP]\r\n\
\r\tshow <ID|version>\r\n\
\r\tsearch <any information>\r\n\
\r\ttelnet <ID|SN|Internet IP> [LAN IP] [Port]\r\n\
\r\texit\r\n"

PTS_CMD_CLIENT_CB_ST g_astCmdClient[PTS_MAX_CLIENT_NUMBER];


U32 pts_get_port_from_string(S8 *szParam, U16 *pusPort)
{
    if (NULL == szParam || NULL == pusPort)
    {
        return DOS_FAIL;
    }

    S8 *szSub = szParam;
    U32 ulPort = 0;

    while (*szSub)
    {
        if (*szSub < '0' || *szSub > '9')
        {
            return DOS_FAIL;
        }
        szSub++;
    }

    ulPort = atoi(szParam);
    if (ulPort > PTS_MAX_PORT)
    {
        return DOS_FAIL;
    }

    *pusPort = ulPort;

    return DOS_SUCC;
}

/**
 * 函数：S32 pts_cmd_client_list_add(S32 lSocket, U32 ulStreamID, U8 *pucPtcID)
 * 功能：
 *      1.添加cmd客户到链表中
 * 参数
 * 返回值:
 */
S32 pts_cmd_client_list_add(S32 lSocket, U32 ulStreamID, U8 *pucPtcID)
{
    S32 i = 0;

    if (NULL == pucPtcID)
    {
        return DOS_FAIL;
    }

    //if (ulStreamID == PT_CTRL_COMMAND)
    //{
    //    g_astCmdClient[0].bIsValid = DOS_TRUE;
    //}

    for (i=0; i<PTS_MAX_CLIENT_NUMBER; i++)
    {
        if (!g_astCmdClient[i].bIsValid)
        {
            g_astCmdClient[i].ulStreamID = ulStreamID;
            g_astCmdClient[i].lSocket = lSocket;
            g_astCmdClient[i].bIsValid = DOS_TRUE;
            dos_memcpy(g_astCmdClient[i].aucID, pucPtcID, PTC_ID_LEN);

            return DOS_SUCC;
        }
    }

    return DOS_FAIL;
}

/**
 * 函数：S32 pts_cmd_client_search_by_socket(S32 lSocket)
 * 功能：根据在socket在链表中查找节点
 * 参数
 * 返回值:
 */
S32 pts_cmd_client_search_by_socket(S32 lSocket)
{

    S32 i = 0;

    for (i=0; i<PTS_MAX_CLIENT_NUMBER; i++)
    {
        if (g_astCmdClient[i].bIsValid && g_astCmdClient[i].lSocket == lSocket)
        {
            return i;
        }
    }

    return DOS_FAIL;
}

/**
 * 函数：S32 pts_cmd_client_search_by_stream(U32 ulStream)
 * 功能：根据stream在链表中查找节点
 * 参数
 * 返回值:
 */
S32 pts_cmd_client_search_by_stream(U32 ulStream)
{

    S32 i = 0;

    for (i=0; i<PTS_MAX_CLIENT_NUMBER; i++)
    {
        if (g_astCmdClient[i].bIsValid && g_astCmdClient[i].ulStreamID == ulStream)
        {
            return i;
        }
    }

    return DOS_FAIL;
}

/**
 * 函数：S32 pts_cmd_client_delete(U32 lSub)
 * 功能：删除节点
 * 参数
 * 返回值:
 */
VOID pts_cmd_client_delete(U32 ulClientIndex)
{
    PT_CC_CB_ST *pstPtcSendNode = NULL;
    PT_MSG_TAG stMsgDes;

    if (ulClientIndex >= PTS_MAX_CLIENT_NUMBER)
    {
        return;
    }

    if (g_astCmdClient[ulClientIndex].bIsValid)
    {
        /* 释放pts收发缓存, 通知ptc释放资源 */
        dos_memcpy(stMsgDes.aucID, g_astCmdClient[ulClientIndex].aucID, PTC_ID_LEN);
        stMsgDes.enDataType = PT_DATA_CMD;
        stMsgDes.ulStreamID = g_astCmdClient[ulClientIndex].ulStreamID;
        pts_delete_stream_addr_node(stMsgDes.ulStreamID);
        pts_set_delete_recv_buff_list_flag(stMsgDes.ulStreamID);
        pthread_mutex_lock(&g_mutexPtcSendList);
        pstPtcSendNode = pt_ptc_list_search(&g_stPtcListSend, g_astCmdClient[ulClientIndex].aucID);
        if (DOS_ADDR_VALID(pstPtcSendNode))
        {
            pts_send_exit_notify_to_ptc(&stMsgDes, pstPtcSendNode);
        }
        pthread_mutex_unlock(&g_mutexPtcSendList);

        pts_delete_send_stream_node(&stMsgDes);
        pts_delete_recv_stream_node(&stMsgDes);
        g_astCmdClient[ulClientIndex].bIsValid = DOS_FALSE;
        g_astCmdClient[ulClientIndex].aucID[0] = '\0';
        g_astCmdClient[ulClientIndex].ulStreamID = U32_BUTT;
        g_astCmdClient[ulClientIndex].lSocket = -1;

        return;
    }


    pt_logr_debug("telnet not found Client, ClientIndex is %d", ulClientIndex);
}

/**
 * 函数：S32 pts_send_ptc_list2cmd_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
 * 功能：命令行中，获取ptc列表的回调函数
 * 参数
 * 返回值:
 */
S32 pts_send_ptc_list2cmd_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
{
    U32 ulClientIndex = *(U32 *)para;

    S8 szPtcListNode[PT_DATA_BUFF_256] = {0};
    dos_snprintf(szPtcListNode, PT_DATA_BUFF_256, "\r%5s%20s%15s%15s%17s%17s%20s\n", column_value[0], column_value[6], column_value[2], column_value[3], column_value[9], column_value[8], column_value[23]);

    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListNode, dos_strlen(szPtcListNode));
    return DOS_SUCC;
}

/**
 * 函数：VOID pts_send_ptc_list2cmd(U32 ulClientIndex)
 * 功能：发送ptc链表到cmd
 * 参数
 * 返回值:
 */
VOID pts_send_ptc_list2cmd(U32 ulClientIndex)
{
    S8 achSql[PTS_SQL_STR_SIZE] = {0};
    S8 szPtcListHead[PT_DATA_BUFF_256] = {0};
    S8 szSeparator[115] = {0};

    memset(szSeparator, '-', 115);
    szSeparator[0] = '\r';
    szSeparator[113] = '\n';
    szSeparator[114] = '\0';

    snprintf(szPtcListHead, sizeof(szPtcListHead), "%5s%20s%15s%15s%17s%17s%20s\n", "ID", "LastLoginTime", "Name", "Remark", "Internet IP", "LAN IP", "MAC");
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListHead, dos_strlen(szPtcListHead));
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szSeparator, dos_strlen(szSeparator));
    sprintf(achSql, "select * from ipcc_alias where register = 1");
    dos_sqlite3_exec_callback(g_pstMySqlite, achSql, pts_send_ptc_list2cmd_callback, &ulClientIndex);
}

VOID pts_send_ptc_list2cmd_by_internetIP(U32 ulClientIndex, S8 *szInternetIP)
{
    S8  achSql[PTS_SQL_STR_SIZE] = {0};
    S8 szPtcListHead[PT_DATA_BUFF_256] = {0};
    S8 szSeparator[115] = {0};

    memset(szSeparator, '-', 115);
    szSeparator[0] = '\r';
    szSeparator[113] = '\n';
    szSeparator[114] = '\0';

    snprintf(szPtcListHead, sizeof(szPtcListHead), "%5s%20s%15s%15s%17s%17s%20s\n", "ID", "LastLoginTime", "Name", "Remark", "Internet IP", "LAN IP", "MAC");
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListHead, dos_strlen(szPtcListHead));
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szSeparator, dos_strlen(szSeparator));

    sprintf(achSql, "select * from ipcc_alias where register = 1 and internetIP = '%s'", szInternetIP);
    dos_sqlite3_exec_callback(g_pstMySqlite, achSql, pts_send_ptc_list2cmd_callback, &ulClientIndex);
}

VOID ptc_fuzzy_search(U32 ulClientIndex, S8 *szKeyWord)
{
    S8 szPtcListFields[10][PT_DATA_BUFF_32] = {"", "lastLoginTime", "name", "remark", "ptcType", "internetIP", "intranetIP", "szMac", "sn", "heartbeatTime"};
    S8 *szSql = NULL;
    S8 *szWhere = NULL;
    S32 i = 0;
    U32 ulLen = 0;
    S8 szSeparator[115] = {0};
    S8 szPtcListHead[PT_DATA_BUFF_256] = {0};

    memset(szSeparator, '-', 115);
    szSeparator[0] = '\r';
    szSeparator[113] = '\n';
    szSeparator[114] = '\0';

    snprintf(szPtcListHead, sizeof(szPtcListHead), "%5s%20s%15s%15s%17s%17s%20s\n", "ID", "LastLoginTime", "Name", "Remark", "Internet IP", "LAN IP", "MAC");
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListHead, dos_strlen(szPtcListHead));
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szSeparator, dos_strlen(szSeparator));

    szSql = (S8 *)dos_dmem_alloc(PT_DATA_BUFF_1024 * 2);
    if (NULL == szSql)
    {
        DOS_ASSERT(0);
        return;
    }
    szWhere = (S8 *)dos_dmem_alloc(PT_DATA_BUFF_1024);
    if (NULL == szWhere)
    {
        DOS_ASSERT(0);
        dos_dmem_free(szSql);
        return;
    }

    dos_memzero(szSql, PT_DATA_BUFF_1024 * 2);
    dos_memzero(szWhere, PT_DATA_BUFF_1024);

    dos_strcpy(szWhere, "(");
    ulLen = dos_strlen("(");

    for (i=1; i<10; i++)
    {
        ulLen += dos_snprintf(szWhere+ulLen, PT_DATA_BUFF_1024-ulLen, "%s LIKE '%%%s%%' OR ", szPtcListFields[i], szKeyWord);
        if (ulLen + 1 >= PT_DATA_BUFF_1024)
        {
            DOS_ASSERT(0);
            dos_dmem_free(szSql);
            dos_dmem_free(szWhere);
            return;
        }
    }

    szWhere[dos_strlen(szWhere) - 3] = '\0';
    ulLen -= 3;
    dos_snprintf(szWhere+ulLen, PT_DATA_BUFF_1024-ulLen, ")");
    dos_snprintf(szSql, PT_DATA_BUFF_1024 * 2, "select * from ipcc_alias where register = 1 and %s", szWhere);

    dos_sqlite3_exec_callback(g_pstMySqlite, szSql, pts_send_ptc_list2cmd_callback, &ulClientIndex);
}


/**
 * 函数：S32 pts_get_password(S8 *szUserName, S8 *pcPassWord, S32 lPasswLen)
 * 功能：获取用户登陆密码
 * 参数
 * 返回值:
 */
S32 pts_get_password(S8 *szUserName, S8 *pcPassWord, S32 lPasswLen)
{
    S8 achSql[PTS_SQL_STR_SIZE] = {0};

    dos_memzero(pcPassWord, lPasswLen);
    sprintf(achSql, "select password from pts_user where name='%s';", szUserName);
    dos_sqlite3_exec_callback(g_pstMySqlite, achSql, pts_get_password_callback, (void *)pcPassWord);
    if (dos_strlen(pcPassWord) > 0)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }

}

S32 pts_look_ptc_detail_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
{
    U32 ulClientIndex = *(U32 *)para;
    S8 szSeparator[100] = {0};

    memset(szSeparator, '-', 100);
    szSeparator[0] = '\r';
    szSeparator[98] = '\n';
    szSeparator[99] = '\0';

    S8 szPtcListNode[PT_DATA_BUFF_256] = {0};
    dos_snprintf(szPtcListNode, PT_DATA_BUFF_256, "%5s%10s%20s%15s%15s%10s%20s\n", "ID", "Online", "LastLoginTime", "Name", "Alias", "Version", "SN");
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListNode, dos_strlen(szPtcListNode));
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szSeparator, dos_strlen(szSeparator));
    if (*column_value[5] == '0')
    {
        dos_snprintf(szPtcListNode, PT_DATA_BUFF_256, "\r%5s%10s%20s%15s%15s%10s%20s\n\n", column_value[0], "N", column_value[6], column_value[2], column_value[3], column_value[4], column_value[1]);
    }
    else
    {
        dos_snprintf(szPtcListNode, PT_DATA_BUFF_256, "\r%5s%10s%20s%15s%15s%10s%20s\n\n", column_value[0], "Y", column_value[6], column_value[2], column_value[3], column_value[4], column_value[1]);
    }

    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListNode, dos_strlen(szPtcListNode));

    dos_snprintf(szPtcListNode, PT_DATA_BUFF_256, "\r%7s%17s%13s%17s%13s%18s%10s\n", "Type", "LAN IP", "Private Port", "Internet IP", "Public Port", "MAC", "RTT(ms)");
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListNode, dos_strlen(szPtcListNode));
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szSeparator, dos_strlen(szSeparator));

    dos_snprintf(szPtcListNode, PT_DATA_BUFF_256, "\r%7s%17s%13s%17s%13s%18s%10s\n\n", column_value[12], column_value[8], column_value[10], column_value[9], column_value[11], column_value[23], column_value[24]);

    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListNode, dos_strlen(szPtcListNode));

    return DOS_SUCC;
}

VOID pts_look_ptc_detail_msg(U32 ulClientIndex, S8 *szPtcID)
{
    if (NULL == szPtcID)
    {
        return;
    }

    S8  achSql[PTS_SQL_STR_SIZE] = {0};
    sprintf(achSql, "select * from ipcc_alias where id = %s;", szPtcID);
    dos_sqlite3_exec_callback(g_pstMySqlite, achSql, pts_look_ptc_detail_callback, &ulClientIndex);
}

/**
 * 函数：S32 cli_server_analyse_cmd(U32 ulClientIndex, U32 ulMode, S8 *szBuffer, U32 ulLength)
 * 功能：分析telnet发送过来的命令
 * 参数：
 *      U32 ulClientIndex：telnet客户端编号
 *      U32 ulMode：当前telnet客户端处于什么模式
 *      S8 *szBuffer：命令缓存
 *      U32 ulLength：命令缓存长度
 * 返回值：
 *      成功返回0，失败返回－1
 */
S32 pts_server_cmd_analyse(U32 ulClientIndex, U32 ulMode, S8 *szBuffer, U32 ulLength)
{
    S8 *pszKeyWord[MAX_KEY_WORDS] = {0};
    S8 szErrorMsg[PT_DATA_BUFF_128] = {0};
    S8 szCMDBak[MAX_CMD_LENGTH] = {0};
    S32 lRet = PT_TELNET_FAIL;
    S32 lKeyCnt = 0;
    S8 *pWord = NULL;
    S32 i=0;
    dos_strncpy(szCMDBak, szBuffer, MAX_CMD_LENGTH);
    szCMDBak[MAX_CMD_LENGTH - 1] = '\0';
    U32 ulStreamID = U32_BUTT;
    S32 lResult = 0;
    S8 szDestIp[IPV6_SIZE] = {0};
    U16 usDestPort = 0;
    S8 achSql[PTS_SQL_STR_SIZE] = {0};
    S8 szSN[PT_DATA_BUFF_64] = {0};

    if (!szBuffer || !ulLength)
    {
        return PT_TELNET_FAIL;
    }
    logr_debug("Cli Server " "CMD " " Control Panel Recv CMD:%s", szBuffer);

    if ('\0' == szBuffer[0])
    {
        return lRet;
    }
    /* 解析命令 */
    pWord = strtok(szBuffer, " ");
    while (pWord)
    {
        pszKeyWord[lKeyCnt] = dos_dmem_alloc(dos_strlen(pWord) + 1);
        if (!pszKeyWord[lKeyCnt])
        {
            DOS_ASSERT(0);
            lRet = PT_TELNET_FAIL;
            goto finished;
        }

        dos_strcpy(pszKeyWord[lKeyCnt], pWord);
        lKeyCnt++;
        pWord = strtok(NULL, " ");
        if (NULL == pWord)
        {
            break;
        }

        if (lKeyCnt >= MAX_KEY_WORDS)
        {
            break;
        }
    }

    if (dos_strncmp(pszKeyWord[0], "help", dos_strlen(pszKeyWord[0])) == 0)
    {
        telnet_send_data(ulClientIndex, MSG_TYPE_CMD, PTS_TELNET_HTLP, dos_strlen(PTS_TELNET_HTLP));

        lRet = PT_TELNET_OTHER;
        goto finished;
    }
    else if (dos_strncmp(pszKeyWord[0], "exit", dos_strlen(pszKeyWord[0])) == 0 || dos_strncmp(pszKeyWord[0], "quit", dos_strlen(pszKeyWord[0])) == 0 )
    {
        lRet = PT_TELNET_EXIT;
        goto finished;
    }
    else if (dos_strncmp(pszKeyWord[0], "list", dos_strlen(pszKeyWord[0])) == 0)
    {
        /* 设备列表 */
        if (lKeyCnt == 1)
        {
            pts_send_ptc_list2cmd(ulClientIndex);
        }
        else if (lKeyCnt == 2)
        {
            if (dos_strcmp(pszKeyWord[1], "?") == 0)
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "Usage : list [Internet IP]\r\n");
                lRet = PT_TELNET_FAIL;
                goto finished;
            }
            else
            {
                pts_send_ptc_list2cmd_by_internetIP(ulClientIndex, pszKeyWord[1]);
            }
        }
        else
        {
            snprintf(szErrorMsg, sizeof(szErrorMsg), "Usage : list [Internet IP]\r\n");
            lRet = PT_TELNET_FAIL;
            goto finished;
        }

        lRet = PT_TELNET_OTHER;
    }
    else if (dos_strncmp(pszKeyWord[0], "show", dos_strlen(pszKeyWord[0])) == 0)
    {
        /* 查看设备详情 */
        if (lKeyCnt != 2)
        {
            snprintf(szErrorMsg, sizeof(szErrorMsg), "Usage : show <ID|version>\r\n");
            lRet = PT_TELNET_FAIL;
            goto finished;
        }
        else
        {
            if (dos_strcmp(pszKeyWord[1], "?") == 0)
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "Usage : show <ID|version>\r\n");
                lRet = PT_TELNET_FAIL;
                goto finished;
            }
            else if (dos_strcmp(pszKeyWord[1], "version") == 0)
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "version : %s\r\n", DOS_PROCESS_VERSION);
                lRet = PT_TELNET_FAIL;
                goto finished;
            }
            else if (pts_is_int(pszKeyWord[1]))
            {
                dos_snprintf(achSql, PTS_SQL_STR_SIZE, "select * from ipcc_alias where id=%s", pszKeyWord[1]);
                if (!dos_sqlite3_record_count(g_pstMySqlite, achSql))  /* 判断是否存在 */
                {
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "PTC is not exit sn(%s)\r\n", pszKeyWord[1]);
                    lRet = PT_TELNET_FAIL;
                    goto finished;
                }
                pts_look_ptc_detail_msg(ulClientIndex, pszKeyWord[1]);
            }
            else
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "  Please input right sn\r\n");
                lRet = PT_TELNET_FAIL;
                goto finished;
            }
        }

        lRet = PT_TELNET_OTHER;
    }
    else if (dos_strncmp(pszKeyWord[0], "search", dos_strlen(pszKeyWord[0])) == 0)
    {
        if (lKeyCnt != 2)
        {
            snprintf(szErrorMsg, sizeof(szErrorMsg), "Usage : search <any information>\r\n");
            lRet = PT_TELNET_FAIL;
            goto finished;
        }
        else
        {
            if (dos_strcmp(pszKeyWord[1], "?") == 0)
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "Usage : search <any information>\r\n");
                lRet = PT_TELNET_FAIL;
                goto finished;
            }
            else
            {
                ptc_fuzzy_search(ulClientIndex, pszKeyWord[1]);
            }
        }

        lRet = PT_TELNET_OTHER;
    }
    else if (dos_strncmp(pszKeyWord[0], "telnet", dos_strlen(pszKeyWord[0])) == 0)
    {
        /* 连接ptc */
        if (lKeyCnt < 2 || (lKeyCnt == 2 && dos_strcmp(pszKeyWord[1], "?") == 0))
        {
            snprintf(szErrorMsg, sizeof(szErrorMsg), "Usage : telnet <ID|SN|Internet IP> [LAN IP] [Port]\r\n");
            lRet = PT_TELNET_FAIL;
            goto finished;
        }
        else
        {
            if (pts_is_ptc_sn(pszKeyWord[1]))
            {
                dos_strncpy(szSN, pszKeyWord[1], PT_DATA_BUFF_64);

                dos_snprintf(achSql, PTS_SQL_STR_SIZE, "select * from ipcc_alias where sn='%s'", szSN);
                if (!dos_sqlite3_record_count(g_pstMySqlite, achSql))  /* 判断是否存在 */
                {
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "  PTC is not exist\r\n");
                    lRet = PT_TELNET_FAIL;
                    goto finished;
                }

                dos_snprintf(achSql, PTS_SQL_STR_SIZE, "select * from ipcc_alias where sn='%s' and register = 1", szSN);
                if (!dos_sqlite3_record_count(g_pstMySqlite, achSql))  /* 是否登陆 */
                {
                    snprintf(szErrorMsg, sizeof(szErrorMsg), " PTC logout\r\n");
                    lRet = PT_TELNET_FAIL;
                    goto finished;
                }

            }
            else if (pts_is_int(pszKeyWord[1]))
            {
                lResult = pts_get_sn_by_id(pszKeyWord[1], szSN, PT_DATA_BUFF_64);
                if (lResult != DOS_SUCC)
                {
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "  Get sn fail by id(%s)\r\n", pszKeyWord[1]);
                    lRet = PT_TELNET_FAIL;
                    goto finished;
                }
            }
            else if(pt_is_or_not_ip(pszKeyWord[1]))
            {
                if (lKeyCnt == 2)
                {
                    dos_strncpy(szDestIp, PTS_TEL_SERV_IP_DEFAULT, IPV6_SIZE);
                }
                else if (lKeyCnt == 3)
                {
                    if (pt_is_or_not_ip(pszKeyWord[2]))
                    {
                        dos_strncpy(szDestIp, pszKeyWord[2], IPV6_SIZE);
                    }
                    else
                    {
                        dos_strncpy(szDestIp, PTS_TEL_SERV_IP_DEFAULT, IPV6_SIZE);
                    }
                }
                else if (lKeyCnt == 4)
                {
                    dos_strncpy(szDestIp, pszKeyWord[2], IPV6_SIZE);
                }
                else
                {
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "Usage : telnet <ID|SN|Internet IP> [LAN IP] [Port]\r\n");
                    lRet = PT_TELNET_FAIL;
                    goto finished;
                }

                lResult = pts_find_ptc_by_dest_addr(pszKeyWord[1], szDestIp, szSN);
                if (lResult != DOS_SUCC)
                {
                    /* 没有找到ptc */
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "  Can not found ptc\r\n");
                    lRet = PT_TELNET_FAIL;
                    goto finished;
                }
            }
            else
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "Usage : telnet <ID|SN|Internet IP> [LAN IP] [Port]\r\n");
                lRet = PT_TELNET_FAIL;
                goto finished;
            }
        }

        if (pts_is_ptc_sn(szSN))
        {
            ulStreamID = pts_create_stream_id();
            lResult = pts_cmd_client_list_add(ulClientIndex, ulStreamID, (U8 *)szSN);
            if (lResult < 0)
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "Too many connection!\r\n");
                lRet = PT_TELNET_FAIL;
                goto finished;
            }

            if (lKeyCnt == 2)
            {
                /* 没有输入IP PORT，使用默认的 */
                dos_strncpy(szDestIp, PTS_TEL_SERV_IP_DEFAULT, IPV6_SIZE);
                usDestPort = PTS_TEL_SERV_PORT_DEFAULT;
            }
            else if (lKeyCnt == 3)
            {
                if (pt_is_or_not_ip(pszKeyWord[2]))
                {
                    dos_strncpy(szDestIp, pszKeyWord[2], IPV6_SIZE);
                    usDestPort = PTS_TEL_SERV_PORT_DEFAULT;
                }
                else if(pts_get_port_from_string(pszKeyWord[2], &usDestPort) == DOS_SUCC)
                {
                    dos_strncpy(szDestIp, PTS_TEL_SERV_IP_DEFAULT, IPV6_SIZE);
                }
                else
                {
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "Usage : telnet <ID|SN|Internet IP> [LAN IP] [Port]\r\n");
                    lRet = PT_TELNET_FAIL;
                    goto finished;
                }
            }
            else if (lKeyCnt == 4)
            {
                if (pt_is_or_not_ip(pszKeyWord[2]))
                {
                    dos_strncpy(szDestIp, pszKeyWord[2], IPV6_SIZE);
                }
                else
                {
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "  IP format error\r\n");
                    lRet = PT_TELNET_FAIL;
                    goto finished;

                }
                if (pts_get_port_from_string(pszKeyWord[3], &usDestPort) != DOS_SUCC)
                {
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "  Port error\r\n");
                    lRet = PT_TELNET_FAIL;
                    goto finished;
                }
            }
            else
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "Usage : telnet <ID|SN|Internet IP> [LAN IP] [Port]\r\n");
                lRet = PT_TELNET_FAIL;
                goto finished;
            }
            pts_save_msg_into_cache((U8 *)szSN, PT_DATA_CMD, ulStreamID, "", 0, szDestIp, usDestPort);

            lRet = PT_TELNET_CONNECT;
        }
        else
        {
             snprintf(szErrorMsg, sizeof(szErrorMsg), "Please input right ptc ID\r\n");
             lRet = PT_TELNET_FAIL;
        }
    }
    else
    {
        snprintf(szErrorMsg, sizeof(szErrorMsg), "Incorrect command.\r\n");
        lRet = PT_TELNET_FAIL;
    }

finished:
    for (i=0; i<lKeyCnt; i++)
    {
        if (pszKeyWord[i])
        {
            dos_dmem_free(pszKeyWord[i]);
        }
    }

    if (lRet < 0)
    {
        if (ulStreamID != U32_BUTT)
        {
            for (i=0; i<PTS_MAX_CLIENT_NUMBER; i++)
            {
                if (g_astCmdClient[i].bIsValid && g_astCmdClient[i].ulStreamID == ulStreamID)
                {
                    g_astCmdClient[i].ulStreamID = U32_BUTT;
                    g_astCmdClient[i].lSocket = -1;
                    g_astCmdClient[i].bIsValid = DOS_FALSE;
                }
            }
        }

        telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szErrorMsg, dos_strlen(szErrorMsg));
    }

    return lRet;

}

VOID pts_telnet_send_msg2ptc(U32 ulClientIndex, S8 *szBuff, U32 ulLen)
{
    S32 lResult = 0;
    S8 szErrorMsg[128] = {0};
    lResult = pts_cmd_client_search_by_socket(ulClientIndex);
    if (lResult < 0)
    {
        //snprintf(szErrorMsg, sizeof(szErrorMsg), "Please input Enter\r\n");
        //telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szErrorMsg, dos_strlen(szErrorMsg));
        snprintf(szErrorMsg, sizeof(szErrorMsg), "\r >");
        telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szErrorMsg, dos_strlen(szErrorMsg));
    }
    else
    {
#if PTS_DEBUG
        S32 i = 0;
        for (i=0; i<ulLen; i++)
        {
            printf("%02x ", (U8)szBuff[i]);
        }
#endif
        pts_save_msg_into_cache(g_astCmdClient[lResult].aucID, PT_DATA_CMD, g_astCmdClient[lResult].ulStreamID, szBuff, ulLen, NULL, 0);
    }
}

/**
 * 函数：VOID pts_send_msg2cmd(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
 * 功能：发送消息到cmd终端
 * 参数
 * 返回值:
 */
VOID pts_send_msg2cmd(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
{
    S32                         lClientSub              = 0;
    U32                         ulArraySub              = 0;
    S8                          *pcSendMsg              = NULL;
    PT_CC_CB_ST                 *pstCCNode              = NULL;
    PT_STREAM_CB_ST             *pstStreamNode          = NULL;
    PT_DATA_TCP_ST              *pstDataTcp             = NULL;
    PT_PTC_COMMAND_ST           *pstPtcCommand          = NULL;
    STREAM_CACHE_ADDR_CB_ST     *pstStreamCacheAddr     = NULL;
    DLL_NODE_S                  *pstListNode            = NULL;
    PTS_CMD_CLIENT_CB_ST        stClientCB;
    S8                          szExitReason[PT_DATA_BUFF_128] = {0};

    lClientSub = pts_cmd_client_search_by_stream(pstNeedRecvNode->ulStreamID);
    if(lClientSub < 0)
    {
        /* 没有找到stream对应的套接字 */
        pt_logr_debug("pts send msg to cmd : error not found stockfd");
        return;
    }
    stClientCB = g_astCmdClient[lClientSub];

    if (pstNeedRecvNode->ExitNotifyFlag)
    {
        /* 响应结束 */
        switch (pstNeedRecvNode->lSeq)
        {
            case 0:
                dos_snprintf(szExitReason, PT_DATA_BUFF_128, "Connection closed by foreign host.\r\nPlease enter any key to continue.");
                break;
            case 1:
                dos_snprintf(szExitReason, PT_DATA_BUFF_128, "Can not connect server.\r\nPlease enter any key to continue.");
                break;
            case 2:
                dos_snprintf(szExitReason, PT_DATA_BUFF_128, "PTC have too many connection.\r\nPlease enter any key to continue.\r\n");
                break;
            case 3:
                dos_snprintf(szExitReason, PT_DATA_BUFF_128, "Timeout.\r\n");
                break;
            default:
                szExitReason[0] = '\0';
                break;
        }

        if (dos_strlen(szExitReason) > 0)
        {
            telnet_send_data(stClientCB.lSocket, MSG_TYPE_CMD_RESPONCE, szExitReason, dos_strlen(szExitReason));
        }
        pts_cmd_client_delete(lClientSub);
        telnet_close_client(stClientCB.lSocket);
        return;
    }

    pthread_mutex_lock(&g_mutexStreamAddrList);
    pstListNode = dll_find(&g_stStreamAddrList, &pstNeedRecvNode->ulStreamID, pts_find_stream_addr_by_streamID);
    if (DOS_ADDR_INVALID(pstListNode))
    {
        pthread_mutex_unlock(&g_mutexStreamAddrList);

        return;
    }

    pstStreamCacheAddr = pstListNode->pHandle;
    if (DOS_ADDR_INVALID(pstStreamCacheAddr))
    {
        pthread_mutex_unlock(&g_mutexStreamAddrList);

        return;
    }

    pstCCNode = pstStreamCacheAddr->pstPtcRecvNode;
    if (DOS_ADDR_INVALID(pstCCNode))
    {
        pthread_mutex_unlock(&g_mutexStreamAddrList);

        return;
    }

    pstStreamNode = pstStreamCacheAddr->pstStreamRecvNode;
    if (DOS_ADDR_INVALID(pstStreamNode))
    {
        pthread_mutex_unlock(&g_mutexStreamAddrList);

        return;
    }

    pthread_mutex_unlock(&g_mutexStreamAddrList);
    pthread_mutex_lock(&pstCCNode->pthreadMutex);

    if (DOS_ADDR_INVALID(pstStreamNode->unDataQueHead.pstDataTcp))
    {
        pt_logr_debug("pts send msg to proxy : not found data queue");
        pthread_mutex_unlock(&pstCCNode->pthreadMutex);

        return;
    }

    while(1)
    {
        /* 发送包，直到不连续 */
        pstStreamNode->lCurrSeq++;
        ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_RECV_CACHE_SIZE - 1);
        pstDataTcp = pstStreamNode->unDataQueHead.pstDataTcp;
        if (pstDataTcp[ulArraySub].lSeq == pstStreamNode->lCurrSeq)
        {
            if (pstNeedRecvNode->ulStreamID == PT_CTRL_COMMAND)
            {
                pstPtcCommand = (PT_PTC_COMMAND_ST *)pstDataTcp[ulArraySub].szBuff;
                cli_out_string(pstPtcCommand->ulIndex, pstDataTcp[ulArraySub].szBuff + sizeof(PT_PTC_COMMAND_ST));
                continue;
            }

            pcSendMsg = pstDataTcp[ulArraySub].szBuff;

            telnet_send_data(stClientCB.lSocket, MSG_TYPE_CMD_RESPONCE, pcSendMsg, pstDataTcp[ulArraySub].ulLen);
            pt_logr_debug("pts send msg to cmd serv len is: %d, stream : %d", pstDataTcp[ulArraySub].ulLen, stClientCB.ulStreamID);
            pts_trace(pstCCNode->bIsTrace, LOG_LEVEL_DEBUG, "pts send msg to cmd : stream : %d, seq : %d, len : %d", stClientCB.ulStreamID, pstDataTcp[ulArraySub].lSeq, pstDataTcp[ulArraySub].ulLen);
        }
        else
        {
            pstStreamNode->lCurrSeq--;
            break;
        }

    } /* end of while(1) */
    pthread_mutex_unlock(&pstCCNode->pthreadMutex);

    return;
}


S32 pts_send_command_to_ptc(U32 ulIndex, S32 argc, S8 **argv)
{
    S32 lResult = 0;
    S8 achSql[PT_DATA_BUFF_512] = {0};
    S8 szSN[PT_DATA_BUFF_64] = {0};
    PT_PTC_COMMAND_ST stCommand;

    if (argc != 3)
    {
        cli_out_string(ulIndex, "Usage : ptsd ptc [ID] [COMMAND].\r\n");

        return 0;
    }

    /* 判断argv[1]对应的ptc 是否存在，是否在线 */
    if (pts_is_ptc_sn(argv[1]))
    {
        dos_strncpy(szSN, argv[1], PT_DATA_BUFF_64);

        dos_snprintf(achSql, PT_DATA_BUFF_512, "select * from ipcc_alias where sn='%s'", szSN);
        if (!dos_sqlite3_record_count(g_pstMySqlite, achSql))  /* 判断是否存在 */
        {
            cli_out_string(ulIndex, "PTC is not exist.\r\n");

            return 0;
        }

        dos_snprintf(achSql, PT_DATA_BUFF_512, "select * from ipcc_alias where sn='%s' and register = 1", szSN);
        if (!dos_sqlite3_record_count(g_pstMySqlite, achSql))  /* 是否登陆 */
        {
            cli_out_string(ulIndex, "PTC is not login.\r\n");

            return 0;
        }

    }
    else if (pts_is_int(argv[1]))
    {
        lResult = pts_get_sn_by_id(argv[1], szSN, PT_DATA_BUFF_64);
        if (lResult != DOS_SUCC)
        {
            cli_out_string(ulIndex, "Get SN fail by ID\r\n");

            return 0;
        }
    }
    else
    {
        cli_out_string(ulIndex, "Usage : ptsd ptc [ID] [COMMAND].\r\n");
    }
    pts_cmd_client_list_add(ulIndex, PT_CTRL_COMMAND, (U8 *)szSN);
    stCommand.ulIndex = ulIndex;
    dos_strncpy(stCommand.szCommand, argv[2], PT_DATA_BUFF_16);
    pts_save_msg_into_cache((U8 *)szSN, PT_DATA_CMD, PT_CTRL_COMMAND, (S8 *)&stCommand, sizeof(PT_PTC_COMMAND_ST), NULL, 0);

    return 0;
}

S32 pts_printf_telnet_msg(U32 ulIndex, S32 argc, S8 **argv)
{
    S32 i = 0;
    U32 ulLen = 0;
    S8 szBuff[PT_DATA_BUFF_512] = {0};

    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%-20s%-15s%-10s\r\n", "SN", "StreamID", "Index");
    cli_out_string(ulIndex, szBuff);

    for (i=0; i<PTS_MAX_CLIENT_NUMBER; i++)
    {
        if (g_astCmdClient[i].bIsValid)
        {
            snprintf(szBuff, sizeof(szBuff), "%.*s%11d%10d\r\n", PTC_ID_LEN, g_astCmdClient[i].aucID, g_astCmdClient[i].ulStreamID, g_astCmdClient[i].lSocket);
            cli_out_string(ulIndex, szBuff);
        }
    }

    return 0;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

