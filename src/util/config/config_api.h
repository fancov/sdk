/**            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  config_api.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: config api给提供的接口函数集合
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include "mxml.h"

#if (INCLUDE_XML_CONFIG)

/* 函数：mxml_node_t * _config_init(S8 *pszFile)
 * 功能：初始化xml配置文件模块，初始化xmldos树
 * 参数：
 *      S8 *pszFile 配置文件文件明
 * 返回值：成功返回dom树指针，失败返回null
 *  */
mxml_node_t * _config_init(S8 *pszFile);

/**
 * 函数：U32 config_deinit(mxml_node_t *pstXMLRoot)
 * 参数：
 *      mxml_node_t *pstXMLRoot：需要被销毁的xml树
 * 功能：销毁xml树
 * 返回值：成功返回0，失败返回－1
 */
U32 _config_deinit(mxml_node_t *pstXMLRoot);

/**
 * 函数：U32 _config_save(mxml_node_t *pstXMLRoot, S8 *pszFile)
 * 功能：将pstXMLRoot指定的xml dom树保存到pszFile指定的文件中文件中
 * 参数：
 *      mxml_node_t *pstXMLRoot：需要保存的对象
 *      S8 *pszFile：文件名
 * 返回值：成功返回0，失败返回－1
 */
U32 _config_save(mxml_node_t *pstXMLRoot, S8 *pszFile);

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
S8  *_config_get_param(mxml_node_t *pstXMLRoot, S8 *path, S8 *name, S8 *szBuff, U32 ulLen);

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
U32 _config_set_param(mxml_node_t *pstXMLRoot, S8 *path, S8 *name, S8 *value);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
