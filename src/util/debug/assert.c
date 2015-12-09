/**            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  assert.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 实现断言信息纪录相关功能
 *     History:
 */

 #ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <pthread.h>
#include <time.h>

/*
 * 定义文件名最大长度
 **/
#define DOS_ASSERT_FILENAME_LEN       64

/**
 * 定义hash表大小
 */
#define DOS_ASSERT_HASH_TABLES_SIZE   256

/**
 * 重定义系统变量类型
 */
typedef struct tm TIME_ST;
typedef time_t    TIME_T_ST;


/**
 * 定义存储assert的节点
 */
typedef struct tagDosAssert
{
    HASH_NODE_S stNode;

    TIME_T_ST     stFirstTime;                             /* 第一次出现的时间 */
    TIME_T_ST     stLastTime;                              /* 最后一次出现的时间 */

    S8      szFilename[DOS_ASSERT_FILENAME_LEN];     /* 文件名 */
    U32     ulLine;                                  /* 行号 */

    U32     ulTimes;                                 /* 出现次数 */
}DOS_ASSERT_NODE_ST;

/**
 * 定义hash桶
 */
static HASH_TABLE_S     *g_pstHashAssert = NULL;

/**
 * 定义线程同步的互斥量
 */
static pthread_mutex_t  g_mutexAssertInfo = PTHREAD_MUTEX_INITIALIZER;

#ifdef INCLUDE_CC_SC
extern U32 sc_log_digest_print(S8 *pszFormat, ...);
#endif

/**
 * 函数: static U32 _assert_string_hash(S8 *pszString, U32 *pulIndex)
 * 功能: 根据字符串计算hash值
 * 参数:
 *      S8 *pszString: 字符串
 *      U32 *pulIndex:返回结果
 * 返回值:成功返回DOS_SUCC,失败返回DOA_FAIL
 */
static U32 _assert_string_hash(S8 *pszString, U32 *pulIndex)
{
    U32  ulHashVal;
    S8  *pszStr;
    U32  i;

    pszStr = pszString;
    if (NULL == pszString || NULL == pulIndex)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashVal = 0;

    for (i = 0; i < dos_strlen(pszStr); i ++)
    {
        ulHashVal += (ulHashVal << 5) + (U8)pszStr[i];
    }

    *pulIndex = ulHashVal % DOS_ASSERT_HASH_TABLES_SIZE;

    return DOS_SUCC;
}


/**
 * 函数: static S32 _assert_find_node(VOID *pSymName, HASH_NODE_S *pNode)
 * 功能: hash表回调函数，在查找节点时用来比较节点是否相同
 * 参数:
 *      VOID *pSymName : 查找的关键字
 *      HASH_NODE_S *pNode : 需要比较的节点
 * 返回值:成功返回DOS_SUCC,失败返回DOA_FAIL
 */
static S32 _assert_find_node(VOID *pSymName, HASH_NODE_S *pNode)
{
    DOS_ASSERT_NODE_ST *pstAssertInfoNode = (DOS_ASSERT_NODE_ST *)pNode;
    S8 szBuff[256] = { 0 };

    dos_snprintf(szBuff, sizeof(szBuff), "%s:%d", pstAssertInfoNode->szFilename, pstAssertInfoNode->ulLine);

    if (0 == dos_strncmp(szBuff, (S8 *)pSymName, sizeof(szBuff)))
    {
        return 0;
    }

    return -1;
}


/**
 * 函数: static VOID _assert_print(HASH_NODE_S *pNode, U32 ulIndex)
 * 功能: 遍历hash表时的回调函数，用来打印相关信息
 * 参数:
 *      HASH_NODE_S *pNode : 需要处理的节点
 *      U32 ulIndex        : 附加参数
 * 返回值: VOID
 */
static VOID _assert_print(HASH_NODE_S *pNode, VOID *pulIndex)
{
    DOS_ASSERT_NODE_ST *pstAssertInfoNode = (DOS_ASSERT_NODE_ST *)pNode;
    U32   ulIndex;
    S8 szBuff[512];
    U32 ulLen;
    TIME_ST *pstTimeFirst, *pstTimeLast;
    S8 szTime1[32] = { 0 }, szTime2[32] = { 0 };

    if (!pNode || !pulIndex)
    {
        return;
    }

    ulIndex = *(U32 *)pulIndex;

    pstTimeFirst = localtime(&pstAssertInfoNode->stFirstTime);
    pstTimeLast = localtime(&pstAssertInfoNode->stLastTime);

    strftime(szTime1, sizeof(szTime1), "%Y-%m-%d %H:%M:%S", pstTimeFirst);
    strftime(szTime2, sizeof(szTime2), "%Y-%m-%d %H:%M:%S", pstTimeLast);

    ulLen = dos_snprintf(szBuff, sizeof(szBuff)
            , "Assert happened %4d times, first time: %s, last time: %s. File:%s, line:%d.\r\n"
            , pstAssertInfoNode->ulTimes
            , szTime1
            , szTime2
            , pstAssertInfoNode->szFilename
            , pstAssertInfoNode->ulLine);
    if (ulLen < sizeof(szBuff))
    {
        szBuff[ulLen] = '\0';
    }
    else
    {
        szBuff[sizeof(szBuff) - 1] = '\0';
    }

    cli_out_string(ulIndex, szBuff);

}

/**
 * 函数: static VOID _assert_print(HASH_NODE_S *pNode, U32 ulIndex)
 * 功能: 遍历hash表时的回调函数，用来打印相关信息
 * 参数:
 *      HASH_NODE_S *pNode : 需要处理的节点
 *      U32 ulIndex        : 附加参数
 * 返回值: VOID
 */
static VOID _assert_record(HASH_NODE_S *pNode, VOID *pParam)
{
    DOS_ASSERT_NODE_ST *pstAssertInfoNode = (DOS_ASSERT_NODE_ST *)pNode;
    S8 szBuff[512];
    U32 ulLen;
    TIME_ST *pstTimeFirst, *pstTimeLast;
    S8 szTime1[32] = { 0 }, szTime2[32] = { 0 };

    if (!pNode)
    {
        return;
    }

    pstTimeFirst = localtime(&pstAssertInfoNode->stFirstTime);
    pstTimeLast = localtime(&pstAssertInfoNode->stLastTime);

    strftime(szTime1, sizeof(szTime1), "%Y-%m-%d %H:%M:%S", pstTimeFirst);
    strftime(szTime2, sizeof(szTime2), "%Y-%m-%d %H:%M:%S", pstTimeLast);

    ulLen = dos_snprintf(szBuff, sizeof(szBuff)
            , "Assert happened %4d times, first time: %s, last time: %s. File:%s, line:%d.\r\n"
            , pstAssertInfoNode->ulTimes
            , szTime1
            , szTime2
            , pstAssertInfoNode->szFilename
            , pstAssertInfoNode->ulLine);
    if (ulLen < sizeof(szBuff))
    {
        szBuff[ulLen] = '\0';
    }
    else
    {
        szBuff[sizeof(szBuff) - 1] = '\0';
    }

    dos_syslog(LOG_LEVEL_ERROR, szBuff);
}



/**
 * 函数: S32 dos_assert_print(U32 ulIndex, S32 argc, S8 **argv)
 * 功能: 命令行回调函数，用来打印相关信息
 * 参数:
 *      U32 ulIndex, : 命令行客户端索引
 *      S32 argc, S8 **argv : 参数集合
 * 返回值: 成功返回0 失败返回-1
 */
S32 dos_assert_print(U32 ulIndex, S32 argc, S8 **argv)
{
    cli_out_string(ulIndex, "Assert Info since the service start:\r\n");

    pthread_mutex_lock(&g_mutexAssertInfo);
    hash_walk_table(g_pstHashAssert,  (VOID *)&ulIndex, _assert_print);
    pthread_mutex_unlock(&g_mutexAssertInfo);

    return 0;
}

/**
 * 函数: S32 dos_assert_record()
 * 功能: 将断言信息记录到文件中
 * 参数:
 * 返回值: 成功返回0 失败返回-1
 */
S32 dos_assert_record()
{
    dos_syslog(LOG_LEVEL_ERROR, "Assert Info since the service start:\r\n");

    pthread_mutex_lock(&g_mutexAssertInfo);
    hash_walk_table(g_pstHashAssert,  NULL, _assert_record);
    pthread_mutex_unlock(&g_mutexAssertInfo);

    return 0;
}


/**
 * 函数: VOID dos_assert(const S8 *pszFileName, const U32 ulLine, const U32 param)
 * 功能: 记录断言
 * 参数:
 *      const S8 *pszFileName, const U32 ulLine, const U32 param ： 断言描述信息
 * 返回值: VOID
 */
DLLEXPORT VOID dos_assert(const S8 *pszFunctionName, const S8 *pszFileName, const U32 ulLine, const U32 param)
{
    U32 ulHashIndex;
    S8 szFileLine[256] = { 0, };
    DOS_ASSERT_NODE_ST *pstFileDescNode = NULL;

#ifdef INCLUDE_CC_SC
    sc_log_digest_print("Assert happened: func:%s,file=%s,line=%d, param:%d"
        , pszFunctionName, pszFileName, ulLine , param);
#endif

    /* 生产hash表key */
    dos_snprintf(szFileLine, sizeof(szFileLine), "%s:%d", pszFileName, ulLine);
    _assert_string_hash(szFileLine, &ulHashIndex);
    //dos_printf("Assert info. Fileline:%s, hash index:%d", szFileLine, ulHashIndex);

    pthread_mutex_lock(&g_mutexAssertInfo);
    pstFileDescNode = (DOS_ASSERT_NODE_ST *)hash_find_node(g_pstHashAssert, ulHashIndex, szFileLine, _assert_find_node);
    if (!pstFileDescNode)
    {
        pstFileDescNode = (DOS_ASSERT_NODE_ST *)dos_dmem_alloc(sizeof(DOS_ASSERT_NODE_ST));
        if (!pstFileDescNode)
        {
            DOS_ASSERT(0);
            pthread_mutex_unlock(&g_mutexAssertInfo);
            return;
        }

        strncpy(pstFileDescNode->szFilename, pszFileName, sizeof(pstFileDescNode->szFilename));
        pstFileDescNode->szFilename[sizeof(pstFileDescNode->szFilename) - 1] = '\0';
        pstFileDescNode->ulLine = ulLine;
        pstFileDescNode->ulTimes = 0;
        time(&pstFileDescNode->stFirstTime);
        time(&pstFileDescNode->stLastTime);
        hash_add_node(g_pstHashAssert, (HASH_NODE_S *)pstFileDescNode, ulHashIndex, NULL);

        //dos_printf("New assert ecord node for fileline:%s, %x", szFileLine, pstFileDescNode);
    }

    pstFileDescNode->ulTimes++;
    time(&pstFileDescNode->stLastTime);
    pthread_mutex_unlock(&g_mutexAssertInfo);
}


/**
 * 函数: S32 dos_assert_init()
 * 功能: 初始化记录断言模块，只要是初始化hash表
 * 参数:
 *      Nan
 * 返回值: 成功返回0 失败返回-1
 */
S32 dos_assert_init()
{
    pthread_mutex_lock(&g_mutexAssertInfo);
    g_pstHashAssert = hash_create_table(DOS_ASSERT_HASH_TABLES_SIZE, NULL);
    if (!g_pstHashAssert)
    {
        pthread_mutex_unlock(&g_mutexAssertInfo);
        return -1;
    }
    pthread_mutex_unlock(&g_mutexAssertInfo);

    return 0;

}

#ifdef __cplusplus
}
#endif /* __cplusplus */

