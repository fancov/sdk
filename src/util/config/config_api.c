/**            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  config_api.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 实现操作xml格式配置文件的api，给予mini－xml库封装
 *              该api分装为线程安全，在外部直接使用.
 *              外部没打开一个xml文档需要建立dos根节点。
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */
#include <dos.h>
#include <pthread.h>
#include "mxml.h"

#if (INCLUDE_XML_CONFIG)

static pthread_mutex_t g_mutexXMLLock = PTHREAD_MUTEX_INITIALIZER;

/**
 * 函数：mxml_node_t * _config_init(S8 *pszFile)
 * 功能：初始化xml配置文件模块，初始化xmldos树
 * 参数：
 *      S8 *pszFile 配置文件文件明
 * 返回值：成功返回dom树指针，失败返回null
 *  */
mxml_node_t * _config_init(S8 *pszFile)
{
    FILE *fp;
    mxml_node_t *pstXMLRoot;

    if (!pszFile || '\0' == pszFile[0])
    {
        dos_printf("%s", "Please special the configuration file.");
        return NULL;
    }

    fp = fopen(pszFile, "r");
    if (!fp)
    {
        dos_printf("%s", "Open configuration file fail.");
        return NULL;
    }

    pthread_mutex_lock(&g_mutexXMLLock);
    pstXMLRoot = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    pthread_mutex_unlock(&g_mutexXMLLock);

    fclose(fp);

    return pstXMLRoot;
}

/**
 * 函数：U32 _config_deinit(mxml_node_t *pstXMLRoot)
 * 参数：
 *      mxml_node_t *pstXMLRoot：需要被销毁的xml树
 * 功能：销毁xml树
 * 返回值：成功返回0，失败返回－1
 */
U32 _config_deinit(mxml_node_t *pstXMLRoot)
{
    pthread_mutex_lock(&g_mutexXMLLock);
    if (pstXMLRoot)
    {
        mxmlDelete(pstXMLRoot);
        pstXMLRoot = NULL;
    }
    pthread_mutex_unlock(&g_mutexXMLLock);

    return 0;
}

/**
 * 函数：U32 _config_save(mxml_node_t *pstXMLRoot, S8 *pszFile)
 * 功能：将pstXMLRoot指定的xml dom树保存到pszFile指定的文件中文件中
 * 参数：
 *      mxml_node_t *pstXMLRoot：需要保存的对象
 *      S8 *pszFile：文件名
 * 返回值：成功返回0，失败返回－1
 */
U32 _config_save(mxml_node_t *pstXMLRoot, S8 *pszFile)
{
    FILE *fp;

    if (!pszFile || '\0' == pszFile[0])
    {
        dos_printf("%s", "Please special the configuration file.");
        return -1;
    }

    pthread_mutex_lock(&g_mutexXMLLock);
    if (!pstXMLRoot)
    {
        dos_printf("%s", "The XML dom is empty.");
        pthread_mutex_unlock(&g_mutexXMLLock);
        return -1;
    }

    fp = fopen(pszFile, "w");
    if (!fp)
    {
        dos_printf("%s", "Open configuration file fail.");
        pthread_mutex_unlock(&g_mutexXMLLock);
        return -1;
    }

    mxmlSaveFile(pstXMLRoot, fp, NULL);
    pthread_mutex_unlock(&g_mutexXMLLock);

    fclose(fp);

    return 0;
}

/**
 *  函数：S8  *_config_get_param(mxml_node_t *pstXMLRoot, S8 *path, S8 *name, S8 *szBuff, U32 ulLen)
 *  功能：获取pstXMLRoot指定的dos树中path路径下，参数名为name的配置项值
 *  参数：
 *      mxml_node_t *pstXMLRoot： 需要查找的dom树
 *      S8 *path： 需要查找的路径
 *      S8 *name：配置项名称
 *      S8 *szBuff：输出参数，返回值buff
 *      U32 ulLen：buff的对打长度
 *  返回值：成功时将value copy到buff中，并且返回buff的首指针；失败时返回null
 */
S8  *_config_get_param(mxml_node_t *pstXMLRoot, S8 *path, S8 *name, S8 *szBuff, U32 ulLen)
{
    mxml_node_t *node;
    const S8 *pValue;

    if (!path || '\0' == path[0])
    {
        dos_printf("%s", "Please special the path.");
        goto error1;
    }

    if (!name || '\0' == name[0])
    {
        dos_printf("%s", "Please special the node name.");
        goto error1;
    }

    if (!szBuff || 0 == ulLen)
    {
        dos_printf("%s", "Please special the node name.");
        goto error1;
    }

    /* 查找路径 */
    pthread_mutex_lock(&g_mutexXMLLock);

    if (!pstXMLRoot)
    {
        dos_printf("%s", "Please special the dom root.");
        goto error2;
    }

    node = mxmlFindPath(pstXMLRoot, path);
    if (!node)
    {
        dos_printf("Cannot read the xml root element for the path:%s.", path);
        goto error2;
    }

    /* 查找路径下的节点 */
    node = mxmlFindElement(node, pstXMLRoot, "param", "name", name, MXML_DESCEND);
    if (!node)
    {
        dos_printf("Cannot read the xml node which has a name %s.", name);
        goto error2;
    }

    /* 获取值 */
    pValue = mxmlElementGetAttr(node, "value");
    /* 可能确实没有值，也需要返回成功 */
    if (!pValue || '\0' == pValue[0])
    {
        szBuff[0] = '\0';
        goto success;
    }
    if (strlen(pValue) > ulLen)
    {
        szBuff[0] = '\0';
        goto error2;
    }

    strncpy(szBuff, pValue, ulLen);
    szBuff[ulLen -1] = '\0';

success:
    pthread_mutex_unlock(&g_mutexXMLLock);
    return szBuff;

error2:
    pthread_mutex_unlock(&g_mutexXMLLock);
error1:
    return NULL;
}

/**
 *  函数：U32 _config_set_param(mxml_node_t *pstXMLRoot, S8 *path, S8 *name, S8 *value)
 *  功能：设置pstXMLRoot指定的dos树中path路径下，参数名为name的配置项值
 *  参数：
 *      mxml_node_t *pstXMLRoot： 需要查找的dom树
 *      S8 *path： 需要查找的路径
 *      S8 *name：配置项名称
 *      S8 *value：配置项值
 *  返回值：回返配置项的值
 *
 *  注意：如果所要设置的配置项不存在，该接口函数将会在指定的目录下创建该参数，并设置值
 */
U32 _config_set_param(mxml_node_t *pstXMLRoot, S8 *path, S8 *name, S8 *value)
{
    mxml_node_t *node;

    if (!pstXMLRoot)
    {
        dos_printf("%s", "Please special the dom tree.");
        return -1;
    }

    if (!path || '\0' == path[0])
    {
        dos_printf("%s", "Please special the path.");
        return -1;
    }

    if (!name || '\0' == name[0])
    {
        dos_printf("%s", "Please special the node name.");
        return -1;
    }

    if (!value)
    {
        dos_printf("%s", "Please special the node value.");
        return -1;
    }

    pthread_mutex_lock(&g_mutexXMLLock);
    node = mxmlFindPath(pstXMLRoot, path);
    if (!node)
    {
        dos_printf("Cannot read the xml root element for the path:%d.", path);
        pthread_mutex_unlock(&g_mutexXMLLock);
        return -1;
    }

    /* 找一下这个路径下，该配置项是否存在，如果不存在就要创建一个，如果存在就直接设置值 */
    node = mxmlFindElement(node, pstXMLRoot, "param", "name", name, MXML_DESCEND);
    if (!node)
    {
        node = mxmlNewElement(node, "param");
        mxmlElementSetAttr(node, "name", name);
        mxmlElementSetAttr(node, "value", value);
    }
    else
    {
        mxmlElementSetAttr(node, "value", value);
    }
    pthread_mutex_unlock(&g_mutexXMLLock);

    return 0;
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
