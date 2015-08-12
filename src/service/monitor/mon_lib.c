#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>
#include <iconv.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include <iconv.h>
#include "mon_lib.h"
#include "mon_def.h"

#ifndef MAX_PID_VALUE
#define MAX_PID_VALUE 65535
#define MIN_PID_VALUE 0

#endif

/* 说明: 后续的定义也必须符合如下格式，3个%s，一个%u, 一个%08x，第一个%s格式化时间，第2个%s格式化MON_NOTIFY_MSG_ST中的szContent成员，第3个%s格式化Notes，%u格式化被报告问题的客户id，
         %08x格式化错误码，而且顺序必须符合该顺序 */
#define NO_ENOUGH_BALANCE_CN "<h1>尊敬的用户，您好：</h1><br/><p style=\"align:2em\">截止%s，你的余额为%s%s，为了不影响你的业务，请尽快充值，谢谢。(CNo:%u, No:%08x)</p>"
#define NO_ENOUGH_BALANCE_EN "<h1>Dear IPCC User:</h1><br/><p style=\"align:2em\">By the time %s,your balance is %s%s,in order to avoid your services being interrupted,please recharge as soon as possible,thank you!(CNo:%u, No:%08x)</p>"

/* 该宏用来定义消息种类的最大个数，可以酌情修改 */
#define MAX_EXCEPT_CNT  4

extern S8 * g_pszAnalyseList;
extern DLL_S *g_pstNotifyList;
extern DB_HANDLE_ST *g_pstCCDBHandle;


/* 消息映射 */
MON_MSG_MAP_ST m_pstMsgMap[][MAX_EXCEPT_CNT] = {
    {
        {"lack_fee",   "余额不足", NO_ENOUGH_BALANCE_CN},
        /* szDesc成员具体需要时具体再定义内容，为了兼容，暂用改格式替代 */
        {"lack_gw" ,   "中继不足", "%s%s%s%u%08x"},
        {"lack_route", "路由不足", "%s%s%s%u%08x"}
    },  /* 编号是0的为简体中文 */
    {
        {"lack_fee",   "No enough balance", NO_ENOUGH_BALANCE_EN},
        {"lack_gw",    "No enough gateway", "%s%s%s%u%08x"},
        {"lack_route", "No enough route", "%s%s%s%u%08x"}
    }   /* 编号是1的是美式英文 */
};


/**
 * 功能:初始化通知消息队列
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32 mon_init_notify_list()
{
    if (DOS_ADDR_INVALID(g_pstNotifyList))
    {
        g_pstNotifyList = (DLL_S *)dos_dmem_alloc(sizeof(DLL_S));
        if (DOS_ADDR_INVALID(g_pstNotifyList))
        {
            mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "Alloc Memory FAIL.");
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
        DLL_Init(g_pstNotifyList);

        return DOS_SUCC;
    }
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "You don\'t need to initialize notification msg list.");
    return DOS_SUCC;
}

/**
 * 函数: U32 mon_get_level(U32 ulNotifyType)
 * 功能: 获取告警对应的最低级别
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回最低级别，失败返回DOS_FAIL
 */
U32 mon_get_level(U32 ulNotifyType)
{
    S8 *pszDesc = NULL;
    S8 szLevel[16] = {0};
    U32 ulLevel;

    pszDesc = m_pstMsgMap[MON_NOTIFY_LANG_CN][ulNotifyType].szName;
    if (config_hb_get_level(pszDesc, szLevel, sizeof(szLevel)) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (0 == dos_strcmp(szLevel, "urgent"))
    {
        ulLevel = MON_NOTIFY_LEVEL_EMERG;
    }
    else if (0 == dos_strcmp(szLevel, "important"))
    {
        ulLevel = MON_NOTIFY_LEVEL_CRITI;
    }
    else if (0 == dos_strcmp(szLevel, "minor"))
    {
        ulLevel = MON_NOTIFY_LEVEL_MINOR;
    }
    else if (0 == dos_strcmp(szLevel, "hint"))
    {
        ulLevel = MON_NOTIFY_LEVEL_HINT;
    }
    else
    {
        ulLevel = MON_NOTIFY_LEVEL_BUTT;
    }

    return ulLevel;
}

/**
 *  函数：U32 mon_notify_customer(MON_NOTIFY_MSG_ST *pstMsg)
 *  功能：将外部告警消息推送给客户
 *  参数：
 *      MON_NOTIFY_MSG_ST *pstMsg  通知消息
 *  返回值： 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 mon_notify_customer(MON_NOTIFY_MSG_ST *pstMsg)
{
    U32 ulLevel = U32_BUTT, ulRet = U32_BUTT, ulType = U32_BUTT;
    S8  szBuff[512] = {0}, szTime[32] = {0};
    S32 lLang = U32_BUTT;

    time_t ulTime = 0;
    struct tm* pstCurTime;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "pstMsg:%p", pstMsg);
        return DOS_FAIL;
    }

    /* 获取该信息的告警级别 */
    ulLevel = mon_get_level(pstMsg->ulWarningID & 0xFF);
    if (MON_NOTIFY_LEVEL_BUTT == ulLevel)
    {
        ulLevel = MON_NOTIFY_LEVEL_EMERG;
    }

    /* 获取告警类型 */
    ulType = pstMsg->ulWarningID & 0xFF;

    /* 获取消息发送时间 */
    ulTime = pstMsg->ulCurTime;
    pstCurTime = localtime(&ulTime);

    /* 获取当前的语言环境 */
    lLang = config_hb_get_lang();
    if (lLang < 0)
    {
        /* 如果配置读取失败，则默认为中文 */
        lLang = MON_NOTIFY_LANG_CN;
    }

    /* 获取收件人相关信息 */
    if ('\0' == pstMsg->stContact.szEmail[0] || '\0' == pstMsg->stContact.szTelNo[0])
    {
        ulRet = mon_get_contact(pstMsg->ulCustomerID, pstMsg->ulRoleID, &pstMsg->stContact);
        if (DOS_SUCC != ulRet)
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
    }

    /* 格式化时间字符串 */
    dos_snprintf(szTime, sizeof(szTime), "%04u-%02u-%02u %02u:%02u:%02u"
                    , pstCurTime->tm_year + 1900
                    , pstCurTime->tm_mon + 1
                    , pstCurTime->tm_mday
                    , pstCurTime->tm_hour
                    , pstCurTime->tm_min
                    , pstCurTime->tm_sec);

    /* 格式化信息 */
    dos_snprintf(szBuff, sizeof(szBuff), m_pstMsgMap[lLang][ulType].szDesc, szTime, pstMsg->szContent, pstMsg->szNotes, pstMsg->ulCustomerID, pstMsg->ulWarningID);

    if (DOS_SUCC != mon_send_email(szBuff, m_pstMsgMap[lLang][ulType].szTitle, pstMsg->stContact.szEmail))
    {
        mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "Send mail FAIL.");
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 *  函数：U32  mon_get_sp_email(S8 *pszEmail)
 *  功能：获取当前运营商邮件地址
 *  参数：
 *      S8 *pszEmail   输出参数，为获取的邮件地址
 *  返回值： 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32  mon_get_sp_email(S8 *pszEmail)
{
    S8 szQuery[] = "SELECT email FROM tbl_contact WHERE customer_id=1 AND name=\'admin\';";

    if (DOS_ADDR_INVALID(pszEmail))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DB_ERR_SUCC != db_query(g_pstCCDBHandle, szQuery, mon_get_sp_email_cb, pszEmail, NULL))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    return DOS_SUCC;
}

/**
 *  函数：S32 mon_get_sp_email_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 *  功能：获取运营商邮件地址的回调函数
 *  参数：
 *  返回值： 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
S32 mon_get_sp_email_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    S8 *pszEmail = (S8 *)pArg;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(pArg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_strncpy(pszEmail, aszValues[0], dos_strlen(aszValues[0]));
    *(pszEmail + dos_strlen(aszValues[0])) = '\0';

    return DOS_SUCC;
}


/**
 *  函数：U32 mon_get_contact(U32 ulCustomerID, U32 ulRoleID, MON_CONTACT_ST *pstContact)
 *  功能：根据客户id与客户角色id获取联系人信息
 *  参数：
 *      U32 ulCustomerID           客户id
 *      U32 ulRoleID               客户角色id
 *      MON_CONTACT_ST *pstContact 输出参数，表示联系人相关信息
 *  返回值： 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
U32 mon_get_contact(U32 ulCustomerID, U32 ulRoleID, MON_CONTACT_ST *pstContact)
{
    S8  szQuery[1024] = {0};
    S32 lRet = 0;

    if (DOS_ADDR_INVALID(pstContact))
    {
        DOS_ASSERT(0);
        mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "Get Contact FAIL. CustomerID:%u; RoleID:%u; pstContact:%p."
                    , ulCustomerID, ulRoleID, pstContact);
        return DOS_FAIL;
    }

    dos_snprintf(szQuery, sizeof(szQuery)
                    , "SELECT tel_number,email FROM tbl_contact WHERE id=%d AND customer_id=%d;"
                    , ulRoleID
                    , ulCustomerID);

    lRet = db_query(g_pstCCDBHandle, szQuery, mon_get_contact_cb, (VOID *)pstContact, NULL);
    if (lRet != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    return DOS_SUCC;
}

/**
 *  函数：S32 mon_get_contact_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 *  功能：获取联系人相关信息回调函数
 *  参数：
 *  返回值： 成功返回DOS_SUCC,失败返回DOS_FAIL
 */
S32 mon_get_contact_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    MON_CONTACT_ST *pstContact = NULL;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(pArg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstContact = (MON_CONTACT_ST *)pArg;

    dos_snprintf(pstContact->szTelNo, sizeof(pstContact->szTelNo), "%s", aszValues[0]);
    dos_snprintf(pstContact->szEmail, sizeof(pstContact->szEmail), "%s", aszValues[1]);

    return DOS_SUCC;
}


/**
 * 功能:为字符串分配内存
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32  mon_init_str_array()
{
   g_pszAnalyseList = (S8 *)dos_dmem_alloc(MAX_TOKEN_CNT * MAX_TOKEN_LEN * sizeof(S8));
   if(DOS_ADDR_INVALID(g_pszAnalyseList))
   {
      DOS_ASSERT(0);
      return DOS_FAIL;
   }
   return DOS_SUCC;
}

/**
 * 功能:释放为字符串分配的内存
 * 参数集：
 *   无参数
 * 返回值：
 *   成功返回DOS_SUCC，失败返回DOS_FAIL
 */
U32  mon_deinit_str_array()
{
   if(DOS_ADDR_INVALID(g_pszAnalyseList))
   {
      DOS_ASSERT(0);
      return DOS_FAIL;
   }
   dos_dmem_free(g_pszAnalyseList);
   g_pszAnalyseList = NULL;
   return DOS_SUCC;
}

/**
 * 功能:判断pszDest是否为pszSrc的子串
 * 参数集：
 *   参数1:S8 * pszSrc  源字符串
 *   参数2:S8 * pszDest 子字符串
 * 返回值：
 *   是则返回DOS_TRUE，否则返回DOS_FALSE
 */
BOOL mon_is_substr(S8 * pszSrc, S8 * pszDest)
{
    S8 * pszStr = pszSrc;

    if(DOS_ADDR_INVALID(pszSrc)
        || DOS_ADDR_INVALID(pszDest))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

   /* 如果pszSrc的长度小于pszDest，那么一定不是子串 */
    if (dos_strlen(pszSrc) < dos_strlen(pszDest))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    for (;pszStr <= pszSrc + dos_strlen(pszSrc) - dos_strlen(pszDest); ++pszStr)
    {
        if (*pszStr == *pszDest)
        {
            if (dos_strncmp(pszStr, pszDest, dos_strlen(pszDest)) == 0)
            {
                return DOS_TRUE;
            }
        }
    }

    return DOS_FALSE;
}

/**
 * 功能:判断pszSentence是否以pszStr结尾
 * 参数集：
 *   参数1:S8 * pszSentence  源字符串
 *   参数2:S8 * pszStr       结尾字符串
 * 返回值：
 *   是则返回DOS_TRUE，否则返回DOS_FALSE
 */
BOOL mon_is_ended_with(S8 * pszSentence, const S8 * pszStr)
{
    S8 *pszSrc = NULL, *pszTemp = NULL;

    if(DOS_ADDR_INVALID(pszSentence)
        || DOS_ADDR_INVALID(pszStr))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    if (dos_strlen(pszSentence) < dos_strlen(pszStr))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }
    pszSrc = pszSentence + dos_strlen(pszSentence) - dos_strlen(pszStr);
    pszTemp = (S8 *)pszStr;

    while (*pszTemp != '\0')
    {
        if (*pszSrc != *pszTemp)
        {
            mon_trace(MON_TRACE_LIB, LOG_LEVEL_ERROR, "\'%s\' is not ended with \'%s\'", pszSrc, pszTemp);
            return DOS_FALSE;
        }
        ++pszSrc;
        ++pszTemp;
    }

   return DOS_TRUE;
}

/**
 * 功能:判断pszFile是否以pszSuffix为文件后缀名
 * 参数集：
 *   参数1:S8 * pszFile   源文件名
 *   参数2:S8 * pszSuffix 文件后缀名
 * 返回值：
 *   是则返回DOS_TRUE，否则返回DOS_FALSE
 */
BOOL mon_is_suffix_true(S8 * pszFile, const S8 * pszSuffix)
{
    S8 * pszFileSrc = NULL;
    S8 * pszSuffixSrc = NULL;

    if(DOS_ADDR_INVALID(pszFile)
        || DOS_ADDR_INVALID(pszFile))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    pszFileSrc = pszFile + dos_strlen(pszFile) -1;
    pszSuffixSrc = (S8 *)pszSuffix + dos_strlen(pszSuffix) - 1;

    for (; pszSuffixSrc >= pszSuffix; pszSuffixSrc--, pszFileSrc--)
    {
        if (*pszFileSrc != *pszSuffixSrc)
        {
            mon_trace(MON_TRACE_LIB, LOG_LEVEL_ERROR, "The suffix of \'%s\' is not \'%s\'.", pszFile, pszSuffix);
            return DOS_FALSE;
        }
    }

    return DOS_TRUE;
}

/**
 * 功能:获取字符串中的一个整数
 * 参数集：
 *   参数1:S8 * pszStr  含有数字字符串的字符串
 * 返回值：
 *   成功则返回字符串中的的一个数字，失败则返回DOS_FAIL
 */
U32 mon_first_int_from_str(S8 * pszStr)
{
    U32  ulData;
    S8   szTail[1024] = {0};
    S8 * pszSrc = pszStr;
    S8 * pszTemp = NULL;
    S32  lRet = 0;

    if(DOS_ADDR_INVALID(pszStr))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    while (!(*pszSrc >= '0' && *pszSrc <= '9'))
    {
        ++pszSrc;
    }
    pszTemp = pszSrc;
    while (*pszTemp >= '0' && *pszTemp <= '9')
    {
        pszTemp++;
    }
    dos_strncpy(szTail, pszSrc, pszTemp - pszSrc);
    szTail[pszTemp - pszSrc] = '\0';

    lRet = dos_atoul(szTail, &ulData);
    if(0 != lRet)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return ulData;
}

/**
 * 功能:将字符串pszStr按照pszRegExpr规则去分割，并存储到pszRsltList中l
 * 参数集：
 *   参数1:S8 * pszStr       含有数字字符串的字符串
 *   参数2:S8* pszRegExpr    分界字符串
 *   参数3:S8* pszRsltList[] 用于存放字符串的首地址
 *   参数4:U32 ulLen         数组最大长度
 * 返回值：
 *   成功则返回DOS_SUCC，失败则返回DOS_FAIL
 */
U32  mon_analyse_by_reg_expr(S8* pszStr, S8* pszRegExpr, S8* pszRsltList[], U32 ulLen)
{
    U32 ulRows = 0;
    S8* pszToken = NULL;

    if(DOS_ADDR_INVALID(pszStr))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if(DOS_ADDR_INVALID(pszRegExpr))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

   /* 每次使用前初始化为0 */
    dos_memzero(g_pszAnalyseList, MAX_TOKEN_CNT * MAX_TOKEN_LEN * sizeof(char));

    for(ulRows = 0; ulRows < ulLen; ulRows++)
    {
        /*把字符串首地址分别置于分配内存的相应位置*/
        pszRsltList[ulRows] = g_pszAnalyseList + ulRows * MAX_TOKEN_LEN;
    }

    for(ulRows = 0; ulRows < ulLen; ulRows++)
    {
        S8 *pszBuff = NULL;
        if(0 == ulRows)
        {
            pszBuff = pszStr;
        }

        pszToken = strtok(pszBuff, pszRegExpr);
        if(DOS_ADDR_INVALID(pszToken))
        {
            break;
        }
        dos_strncpy(pszRsltList[ulRows], pszToken, dos_strlen(pszToken));
        pszRsltList[ulRows][dos_strlen(pszToken)] = '\0';
    }

    return DOS_SUCC;
}

/**
 * 功能:生成告警id
 * 参数集：
 *   参数1:U32 ulResType     资源种类
 *   参数2:U32 ulNo          资源编号
 *   参数3:U32 ulErrType     错误类型
 * 返回值：
 *   成功则返回告警id，失败则返回(U32)0xff，其中系统告警最高位是1，其它告警最高位是0
 */
U32 mon_generate_warning_id(U32 ulResType, U32 ulNo, U32 ulErrType)
{
    if(ulResType >= (U32)0xff || ulNo >= (U32)0xff
        || ulErrType >= (U32)0xff)
    {
        DOS_ASSERT(0);
        return (U32)0xff;
    }
    /* 第1个8位存储资源类型，第2个8位存储资源编号，第3个8位存储错误编号 */
    return ((ulResType << 24) & 0xff000000) | ((ulNo << 16) & 0xff0000) | ulErrType;
}


/**
 * 功能:为字符串去头去尾，只留下最简单的名字
 * 参数集：
 *   参数1:S8 * pszCmd   进程启动命令
 * 返回值：
 *   成功则返回去头去尾之后的简单名称，失败则返回NULL
 */
S8 * mon_str_get_name(S8 * pszCmd)
{
    S8 * pszPtr = pszCmd;
    if(DOS_ADDR_INVALID(pszPtr))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /**
      *  找到第一个空格，前面为命令的绝对路径，后面为命令附带的相关参数
      */
    while(*pszPtr != ' ' && *pszPtr != '\0' && *pszPtr != '\t')
    {
        ++pszPtr;
    }
    *pszPtr = '\0';

    /*找到最后一个'/'字符，'/'和' '之间的部分是命令的最简化名称*/
    while(*(pszPtr - 1) != '/' && pszPtr != pszCmd)
    {
        --pszPtr;
    }

    pszCmd = pszPtr;

    return pszCmd;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

