#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if INCLUDE_HTTP_API

#include <http_api.h>

#if INCLUDE_RES_MONITOR
#include "../../src/service/mon_def.h"
extern  DB_HANDLE_ST * g_pstDBHandle;
#endif

#if DEBUG
U32 http_api_test_handle(HTTP_HANDLE_ST *pstHandle, list_t *pstArgvList)
{
    list_t *pstListNode = NULL;
    API_ARGV_ST *pstArgv;
    S32 lCnt = 0;
    S8 *pszVal;

    if (!pstArgvList)
    {
        http_api_write(pstHandle, "Just a test. There is no data!");
        return DOS_SUCC;
    }

    pszVal = http_api_get_var(pstArgvList, "id");
    http_api_write(pstHandle, "Found an parameter. Name: id, value: %s!", pszVal);

    pstListNode = pstArgvList->next;
    while (pstListNode != pstArgvList)
    {
        pstArgv = dos_list_entry(pstListNode, API_ARGV_ST, stList);
        if (!pstArgv)
        {
            continue;
        }

        lCnt++;
        http_api_write(pstHandle, "Test. Parameter %d, Name: %s, Value: %s <br>", lCnt, pstArgv->pszName, pstArgv->pszVal);

        pstListNode = pstListNode->next;
        if (!pstListNode)
        {
            break;
        }
    }
    http_api_write(pstHandle, "Just a test. Parameters : %d!", lCnt);

    return DOS_SUCC;
}
#endif

#if INCLUDE_RES_MONITOR
/**
 *  函数: U32 http_api_alarm_clear(HTTP_HANDLE_ST *pstHandle, list_t *pstArgvList);
 *  参数:
 *        HTTP_HANDLE_ST *pstHandle  HTTP请求句柄
 *        list_t *pstArgvList        请求参数列表
 *  功能: 处理告警消除的HTTP请求
 *  返回: 成功返回DOS_SUCC，失败返回DOS_FAIL
 **/
U32 http_api_alarm_clear(HTTP_HANDLE_ST *pstHandle, list_t *pstArgvList)
{
    S8  *pszAlarmLogID = NULL;
    S8   szQuery[256] = {0};
    U32  ulAlarmLogID = U32_BUTT;

    pszAlarmLogID = http_api_get_var(pstArgvList, "alarmlog_id");
    if (DOS_ADDR_INVALID(pszAlarmLogID))
    {
        http_api_write(pstHandle, "Get alarmlog_id FAIL.alarmlog_id:%p. <br>", pszAlarmLogID);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (dos_atoul(pszAlarmLogID, &ulAlarmLogID) < 0)
    {
        http_api_write(pstHandle, "Convert Alarm ID string to Integer FAIL. AlarmLogID:%s <br>", pszAlarmLogID);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "UPDATE tbl_alarmlog SET status=2 WHERE id=%u;", ulAlarmLogID);
    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) != DB_ERR_SUCC)
    {
        http_api_write(pstHandle, "Execute SQL FAIL. SQL:%s <br>", szQuery);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    http_api_write(pstHandle, "Clear Alarm SUCC.<br>");

    return DOS_SUCC;
}

/**
 *  函数: U32 http_api_system_restart(HTTP_HANDLE_ST *pstHandle, list_t *pstArgvList);
 *  参数:
 *        HTTP_HANDLE_ST *pstHandle  HTTP请求句柄
 *        list_t *pstArgvList        请求参数列表
 *  功能: 重启系统
 *  返回: 成功返回DOS_SUCC，失败返回DOS_FAIL
 **/
U32 http_api_system_restart(HTTP_HANDLE_ST *pstHandle, list_t *pstArgvList)
{
    S8  *pszStyle = NULL, *pszTimeStamp = NULL;
    U32 ulStyle = U32_BUTT, ulTimeStamp = U32_BUTT, ulRet = U32_BUTT;

    pszStyle = http_api_get_var(pstArgvList, "style");
    pszTimeStamp = http_api_get_var(pstArgvList, "time");

    if (dos_atoul(pszTimeStamp, &ulTimeStamp) < 0)
    {
        http_api_write(pstHandle, "Get Param \'time\' FAIL.(TimeStamp:%s)<br>", pszTimeStamp);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (0 == dos_strnicmp(pszStyle, "immediate", dos_strlen("immediate")))
    {
        if (0 != ulTimeStamp)
        {
            http_api_write(pstHandle, "%s", "Param \'time\' must be \'0\' if you restart system immediately.<br>");
            return DOS_FAIL;
        }
        http_api_write(pstHandle, "%s", "The System will be restarted immediately.<br>");
        ulStyle = MON_SYS_RESTART_IMMEDIATELY;
    }
    else if (0 == dos_strnicmp(pszStyle, "fixed", dos_strlen("fixed")))
    {
        if (0 == ulTimeStamp || U32_BUTT == ulTimeStamp)
        {
            http_api_write(pstHandle, "Param \'time\' shouldn\'t be \'0\' or \'%u\' if you restart system at fixed time.<br>", U32_BUTT);
            return DOS_FAIL;
        }
        http_api_write(pstHandle, "The System will be restarted in timestamp %u.<br>", ulTimeStamp);
        ulStyle = MON_SYS_RESTART_FIXED;
    }
    else if (0 == dos_strnicmp(pszStyle, "later", dos_strlen("later")))
    {
        if (0 != ulTimeStamp)
        {
            http_api_write(pstHandle, "%s", "Param \'time\' must be \'0\' if you restart system when no service runs.<br>");
            return DOS_FAIL;
        }
        http_api_write(pstHandle, "%s", "The System will be restarted later.<br>");
        ulStyle = MON_SYS_RESTART_LATER;
    }
    else
    {
        http_api_write(pstHandle, "Sorry, the server doesn\'t support param \'%s\',please check it.<br>", pszStyle);
        return DOS_FAIL;
    }

    ulRet = mon_restart_system(ulStyle, ulTimeStamp);
    if (DOS_SUCC != ulRet)
    {
        http_api_write(pstHandle, "Restart System FAIL.(Style:%u, TimeStamp:%u).", ulStyle, ulTimeStamp);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

#endif

U32 http_api_handle_init()
{
#if DEBUG
    http_api_handle_reg("/test", http_api_test_handle);
#endif
#if INCLUDE_LICENSE_SERVER
    http_api_handle_reg("/generate", http_lics_generate);
#endif
#if INCLUDE_RES_MONITOR
    http_api_handle_reg("/alarmlog", http_api_alarm_clear);
    http_api_handle_reg("/system" , http_api_system_restart);
#endif
    return DOS_SUCC;
}



#endif /* end of INCLUDE_HTTP_API */

#ifdef __cplusplus
}
#endif /* __cplusplus */


