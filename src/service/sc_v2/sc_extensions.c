/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_ep_extensions.c
 *
 *  创建时间: 2015年6月18日15:25:18
 *  作    者: Larry
 *  描    述: 处理FS核心发过来的各种事件
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <esl.h>
#include <sys/time.h>
#include <pthread.h>
#include <bs_pub.h>
#include <libcurl/curl.h>
#include "sc_def.h"
#include "sc_db.h"
#include "sc_res.h"
#include "sc_debug.h"

/** 接收ESL事件线程 */
pthread_t             g_stESLExtEventRecv;

/** 处理ESL事件线程 */
pthread_t             g_stESLExtEventProc;

/** 是否要求退出 */
BOOL                  g_blESLExtEventRunning = DOS_FALSE;

/** esl 句柄 */
esl_handle_t          g_stESLExtRecvHandle;

DLL_S            g_stExtMngtMsg;
pthread_mutex_t  g_mutexExtMngtMsg = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t   g_condExtMngtMsg  = PTHREAD_COND_INITIALIZER;
pthread_t        g_pthExtMngtProcTask;

#define SC_ESL_EXT_EVENT_LIST "custom sofia::register_attempt sofia::gateway_state sofia::expire sofia::unregister"

#define MAX_BUFFER_LEN   100

typedef struct tagEPData{
    S8 *pszSUBClass;
    S8 *pszGateway;
    S8 *pszStatus;
    S8 *pszFromUser;
    S8 *pszUser;
    S8 *pszExpire;
    S8 *pszNetworkIP;
    S8 *pszContact;
    S8 *pszAuthResult;
}SC_EXT_DATA_ST;

extern U32 sc_esl_execute_cmd(const S8 *pszCmd, S8 *pszUUID, U32 ulLenght);

VOID sc_esl_ext_event_process(SC_EXT_DATA_ST *pstExtData)
{
    U32                     ulGateWayID  = 0;
    U32                     ulResult     = DOS_FAIL;
    U32                     ulSipID      = 0;
    U32                     ulPublicIP   = 0;
    U32                     ulPrivateIP  = 0;
    S8                      *pszUser     = NULL;
    S8                      szPrivateIP[17] = {0};
    SC_TRUNK_STATE_TYPE_EN  enTrunkState;
    SC_SIP_STATUS_TYPE_EN   enStatus;

    if (DOS_ADDR_INVALID(pstExtData))
    {
        DOS_ASSERT(0);
        return;
    }

    if (DOS_ADDR_INVALID(pstExtData->pszSUBClass) || '\0' == pstExtData->pszSUBClass)
    {
         sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "%s", "Not get msg type");

         goto end;
    }

    if (dos_strcmp(pstExtData->pszSUBClass, "sofia::gateway_state") == 0)
    {
        /* gateway_state 维护中继的状态 */
        if (DOS_ADDR_INVALID(pstExtData->pszGateway)
            || '\0' == pstExtData->pszGateway
            || dos_atoul(pstExtData->pszGateway, &ulGateWayID) < 0)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "%s", "Not get Gateway");
            goto end;
        }

        if (DOS_ADDR_INVALID(pstExtData->pszStatus) || '\0' == pstExtData->pszStatus)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "%s", "Not get State");
            goto end;
        }

        if (dos_strcmp(pstExtData->pszStatus, "UNREGED") == 0)
        {
            enTrunkState = SC_TRUNK_STATE_TYPE_UNREGED;
        }
        else if (dos_strcmp(pstExtData->pszStatus, "TRYING") == 0)
        {
            enTrunkState = SC_TRUNK_STATE_TYPE_TRYING;
        }
        else if (dos_strcmp(pstExtData->pszStatus, "REGISTER") == 0)
        {
            enTrunkState = SC_TRUNK_STATE_TYPE_REGISTER;
        }
        else if (dos_strcmp(pstExtData->pszStatus, "REGED") == 0)
        {
            enTrunkState = SC_TRUNK_STATE_TYPE_REGED;
        }
        else if (dos_strcmp(pstExtData->pszStatus, "FAILED") == 0)
        {
            enTrunkState = SC_TRUNK_STATE_TYPE_FAILED;
        }
        else if (dos_strcmp(pstExtData->pszStatus, "FAIL_WAIT") == 0)
        {
            enTrunkState = SC_TRUNK_STATE_TYPE_FAIL_WAIT;
        }
        else if (dos_strcmp(pstExtData->pszStatus, "NOREG") == 0)
        {
            enTrunkState = SC_TRUNK_STATE_TYPE_NOREG;
        }
        else
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "Trunk state matching FAIL. %s", pstExtData->pszStatus);
            goto end;
        }

        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "Trunk state matching SUCC. %s, %u", pstExtData->pszStatus, enTrunkState);

        /* 更新控制块状态 */
        ulResult = sc_gateway_update_status(ulGateWayID, enTrunkState);
        if (ulResult != DOS_SUCC)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "update trunk(%u) CB fail.", ulGateWayID);
        }
        /* 更新数据库状态 */
        ulResult = sc_gateway_update_status2db(ulGateWayID, enTrunkState);
        if(DB_ERR_SUCC != ulResult)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "update trunk(%u) db fail.", ulGateWayID);
        }

        goto end;
    }
    else if (dos_strcmp(pstExtData->pszSUBClass, "sofia::register_attempt") == 0
        || dos_strcmp(pstExtData->pszSUBClass, "sofia::unregister") == 0
        || dos_strcmp(pstExtData->pszSUBClass, "sofia::expire") == 0)
    {
        /* TODO 维护ACD模块中坐席所对应的SIP分机的状态 */
        /* 维护SIP分机的状态 */

        pszUser = pstExtData->pszFromUser;
        if (DOS_ADDR_INVALID(pszUser) || '\0' == pszUser)
        {
             sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "%s", "Not get userid");
             goto end;
        }

        if (dos_strcmp(pstExtData->pszSUBClass, "sofia::register_attempt") == 0)
        {
            /* 判断 expires 字段，如果为0，则为取消注册，反之为注册消息 */
            if (DOS_ADDR_INVALID(pstExtData->pszExpire) || '\0' == pstExtData->pszExpire)
            {
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "%s", "Not get expires");
                goto end;
            }

            if (pstExtData->pszExpire[0] == '0')
            {
                enStatus = SC_SIP_STATUS_TYPE_UNREGISTER;
            }
            else
            {
                if (DOS_ADDR_VALID(pstExtData->pszAuthResult) && dos_strncmp(pstExtData->pszAuthResult, "FORBIDDEN", dos_strlen("FORBIDDEN")) == 0)
                {
                    enStatus = SC_SIP_STATUS_TYPE_UNREGISTER;
                }
                else
                {
                    enStatus = SC_SIP_STATUS_TYPE_REGISTER;
                }
            }
        }
        else
        {
            /* sofia::unregister/sofia::expire */
            enStatus = SC_SIP_STATUS_TYPE_UNREGISTER;

            if (dos_strcmp(pstExtData->pszSUBClass, "sofia::expire") == 0)
            {
                pszUser = pstExtData->pszUser;
            }
        }

        ulResult = sc_sip_account_update_status(pszUser, enStatus, &ulSipID);
        if (ulResult != DOS_SUCC)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "update sip(userid : %s) status fail", pszUser);

            goto end;
        }

        /* 将SIP的IP地址存到表tbl_sip中 */
        if (DOS_ADDR_INVALID(pstExtData->pszNetworkIP) || '\0' == pstExtData->pszNetworkIP)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "%s", "Not get network-ip");
            ulPublicIP = 0;
        }
        else
        {
            dos_strtoipaddr(pstExtData->pszNetworkIP, (VOID *)(&ulPublicIP));
        }

        if (DOS_ADDR_INVALID(pstExtData->pszContact) || '\0' == pstExtData->pszContact)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "%s", "Not get contact");
            ulPrivateIP = 0;
        }
        else
        {
            if (1 == dos_sscanf(pstExtData->pszContact, "%*[^@]@%[^:]", szPrivateIP))
            {
                dos_strtoipaddr(szPrivateIP, (VOID *)(&ulPrivateIP));
            }
            else
            {
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "%s", "Not get private IP from contact");
                ulPrivateIP = 0;
            }
        }

        ulResult = sc_sip_account_update_info2db(ulPublicIP, ulPrivateIP, enStatus, ulSipID);
        if(DB_ERR_SUCC != ulResult)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "update sip db fail, userid is %s", pszUser);
        }
    }
end:
    if (DOS_ADDR_VALID(pstExtData))
    {
        if (pstExtData->pszSUBClass)
        {
            dos_dmem_free(pstExtData->pszSUBClass);
        }

        if (pstExtData->pszGateway)
        {
            dos_dmem_free(pstExtData->pszGateway);
        }

        if (pstExtData->pszStatus)
        {
            dos_dmem_free(pstExtData->pszStatus);
        }

        if (pstExtData->pszFromUser)
        {
            dos_dmem_free(pstExtData->pszFromUser);
        }

        if (pstExtData->pszUser)
        {
            dos_dmem_free(pstExtData->pszUser);
        }

        if (pstExtData->pszAuthResult)
        {
            dos_dmem_free(pstExtData->pszAuthResult);
        }

        if (pstExtData->pszExpire)
        {
            dos_dmem_free(pstExtData->pszExpire);
        }

        if (pstExtData->pszNetworkIP)
        {
            dos_dmem_free(pstExtData->pszNetworkIP);
        }

        if (pstExtData->pszContact)
        {
            dos_dmem_free(pstExtData->pszContact);
        }
    }

    return;
}

VOID* sc_ext_mgnt_runtime(VOID *ptr)
{
    DLL_NODE_S              *pstListNode = NULL;
    struct timespec         stTimeout;
    SC_EXT_DATA_ST          *pstExtData  = NULL;
    SC_PTHREAD_MSG_ST   *pstPthreadMsg = NULL;

    pstPthreadMsg = dos_pthread_cb_alloc();
    if (DOS_ADDR_VALID(pstPthreadMsg))
    {
        pstPthreadMsg->ulPthID = pthread_self();
        pstPthreadMsg->func = sc_ext_mgnt_runtime;
        pstPthreadMsg->pParam = ptr;
        dos_strcpy(pstPthreadMsg->szName, "sc_ext_mgnt_runtime");
    }

    for (;;)
    {
        if (DOS_ADDR_VALID(pstPthreadMsg))
        {
            pstPthreadMsg->ulLastTime = time(NULL);
        }

        pthread_mutex_lock(&g_mutexExtMngtMsg);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condExtMngtMsg, &g_mutexExtMngtMsg, &stTimeout);
        pthread_mutex_unlock(&g_mutexExtMngtMsg);

        while (1)
        {
            if (DLL_Count(&g_stExtMngtMsg) <= 0)
            {
                break;
            }

            pthread_mutex_lock(&g_mutexExtMngtMsg);
            pstListNode = dll_fetch(&g_stExtMngtMsg);
            pthread_mutex_unlock(&g_mutexExtMngtMsg);
            if (DOS_ADDR_INVALID(pstListNode))
            {
                DOS_ASSERT(0);
                break;
            }

            if (DOS_ADDR_INVALID(pstListNode->pHandle))
            {
                DOS_ASSERT(0);
                continue;
            }

            pstExtData = (SC_EXT_DATA_ST *)pstListNode->pHandle;
            if (DOS_ADDR_INVALID(pstExtData))
            {
                dos_dmem_free(pstListNode);
                pstListNode = NULL;
                continue;
            }

            pstListNode->pHandle = NULL;
            dos_dmem_free(pstListNode);

            sc_esl_ext_event_process(pstExtData);

            dos_dmem_free(pstExtData);
            pstExtData = NULL;
        }
    }
}

VOID* sc_ext_recv_runtime(VOID *ptr)
{
    U32                  ulRet          = ESL_FAIL;
    BOOL                 bFirstConn     = DOS_TRUE;
    DLL_NODE_S           *pstDLLNode    = NULL;
    esl_event_t          *pstEvent      = NULL;
    SC_EXT_DATA_ST       *pstExtData    = NULL;
    //SC_PTHREAD_MSG_ST   *pstPthreadMsg  = NULL;

    g_blESLExtEventRunning = DOS_TRUE;

#if 0
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    pstPthreadMsg = sc_pthread_cb_alloc();
    if (DOS_ADDR_VALID(pstPthreadMsg))
    {
        pstPthreadMsg->ulPthID = pthread_self();
        pstPthreadMsg->func = sc_ext_recv_runtime;
        pstPthreadMsg->pParam = ptr;
        dos_strcpy(pstPthreadMsg->szName, "sc_ext_recv_runtime");
    }
#endif

    for (;;)
    {
        /* 如果退出标志被置上，就准备退出了 */
        if (!g_blESLExtEventRunning)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "Event process exit flag has been set. the task will be exit.");
            break;
        }

        /*
         * 检查连接是否正常
         * 如果连接不正常，就准备重连
         **/
        if (!g_stESLExtRecvHandle.connected)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "ELS for event connection has been down, re-connect. %s", __FUNCTION__);
            g_stESLExtRecvHandle.event_lock = 1;
            ulRet = esl_connect(&g_stESLExtRecvHandle, "127.0.0.1", 8021, NULL, "ClueCon");
            if (ESL_SUCCESS != ulRet)
            {
                esl_disconnect(&g_stESLExtRecvHandle);
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "ELS for event re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_stESLExtRecvHandle.err);

                sleep(1);
                continue;
            }

            esl_global_set_default_logger(ESL_LOG_LEVEL_WARNING);
            esl_events(&g_stESLExtRecvHandle, ESL_EVENT_TYPE_PLAIN, SC_ESL_EXT_EVENT_LIST);

            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EXT_MNGT), "%s", "ELS for event connect Back to Normal.");
        }

        //if (DOS_ADDR_VALID(pstPthreadMsg))
        //{
        //    pstPthreadMsg->ulLastTime = time(NULL);
        //}

        if (bFirstConn)
        {
            bFirstConn = DOS_FALSE;
            sc_esl_execute_cmd("api reloadxml\r\n", NULL, 0);
        }

        ulRet = esl_recv_event(&g_stESLExtRecvHandle, 1, NULL);
        if (ESL_FAIL == ulRet)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EXT_MNGT), "%s", "ESL Recv event fail, continue.");
            esl_disconnect(&g_stESLExtRecvHandle);
            continue;
        }

        pstEvent = g_stESLExtRecvHandle.last_ievent;
        if (DOS_ADDR_INVALID(pstEvent))
        {
            DOS_ASSERT(0);

            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EXT_MNGT), "%s", "ESL get event fail, continue.");
            continue;
        }

        pstDLLNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            DOS_ASSERT(0);

            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EXT_MNGT), "ESL recv thread recv event %s(%d). Alloc memory fail. Drop"
                            , esl_event_get_header(pstEvent, "Event-Name")
                            , pstEvent->event_id);

            continue;
        }

        pstExtData = dos_dmem_alloc(sizeof(SC_EXT_DATA_ST));
        if (DOS_ADDR_INVALID(pstExtData))
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EXT_MNGT), "Alloc memory fail. %s:%u", __FILE__, __LINE__);
            continue;
        }

        pstExtData->pszSUBClass = dos_strndup(esl_event_get_header(pstEvent, "Event-Subclass"), MAX_BUFFER_LEN);
        pstExtData->pszGateway = dos_strndup(esl_event_get_header(pstEvent, "Gateway"), MAX_BUFFER_LEN);
        pstExtData->pszStatus = dos_strndup(esl_event_get_header(pstEvent, "State"), MAX_BUFFER_LEN);
        pstExtData->pszFromUser = dos_strndup(esl_event_get_header(pstEvent, "from-user"), MAX_BUFFER_LEN);
        pstExtData->pszExpire = dos_strndup(esl_event_get_header(pstEvent, "expires"), MAX_BUFFER_LEN);
        pstExtData->pszNetworkIP = dos_strndup(esl_event_get_header(pstEvent, "network-ip"), MAX_BUFFER_LEN);
        pstExtData->pszContact = dos_strndup(esl_event_get_header(pstEvent, "contact"), MAX_BUFFER_LEN);
        pstExtData->pszAuthResult = dos_strndup(esl_event_get_header(pstEvent, "auth-result"), MAX_BUFFER_LEN);
        pstExtData->pszUser = dos_strndup(esl_event_get_header(pstEvent, "user"), MAX_BUFFER_LEN);

        pthread_mutex_lock(&g_mutexExtMngtMsg);

        DLL_Init_Node(pstDLLNode);
        pstDLLNode->pHandle = pstExtData;
        DLL_Add(&g_stExtMngtMsg, pstDLLNode);
        pthread_cond_signal(&g_condExtMngtMsg);
        pthread_mutex_unlock(&g_mutexExtMngtMsg);
    }

    return NULL;
}

/* 初始化事件处理模块 */
U32 sc_ext_mngt_init()
{
    DLL_Init(&g_stExtMngtMsg);

    return DOS_SUCC;
}

/* 启动事件处理模块 */
U32 sc_ext_mngt_start()
{
    if (pthread_create(&g_stESLExtEventRecv, NULL, sc_ext_mgnt_runtime, NULL) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (pthread_create(&g_stESLExtEventRecv, NULL, sc_ext_recv_runtime, NULL) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/* 停止事件处理模块 */
U32 sc_ext_mngt_stop()
{
    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


