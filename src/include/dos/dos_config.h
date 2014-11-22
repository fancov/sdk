/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_config.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: 统一配置项管理，由使用者自己维护
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if (INCLUDE_XML_CONFIG)

/**
 * 函数：U32 config_hh_get_send_interval()
 * 功能：获取心跳发送间隔
 * 参数：
 * 返回值：成功返回心跳间隔.失败返回0
 */
U32 config_hh_get_send_interval();


/**
 * 函数：U32 config_hb_get_max_fail_cnt()
 * 功能：获取发送最大失败次数
 * 参数：
 * 返回值：成功返回最大失败次数.失败返回0
 */
U32 config_hb_get_max_fail_cnt();

/**
 * 函数：S32 config_hb_get_treatment()
 * 功能：获取心跳超时的处理方式
 * 参数：
 * 返回值：成功返回处理方式编号.失败返回－1
 */
S32 config_hb_get_treatment();

/**
 * 函数：S8* config_get_service_root(S8 *pszBuff, U32 ulLen)
 * 功能：获取服务程序的根目录
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回buff的指针，失败时返回null
 */
S8* config_get_service_root(S8 *pszBuff, U32 ulLen);

/**
 * 函数：U32 config_get_mysql_host(S8 *pszBuff, U32 ulLen)
 * 功能：获取数据库主机名
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_mysql_host(S8 *pszBuff, U32 ulLen);

/**
 * 函数：U32 config_get_mysql_host()
 * 功能：获取数据库主机名
 * 参数：
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_mysql_port();

/**
 * 函数：U32 config_get_mysql_user(S8 *pszBuff, U32 ulLen)
 * 功能：获取数据库主机名
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_mysql_user(S8 *pszBuff, U32 ulLen);

/**
 * 函数：U32 config_get_mysql_password(S8 *pszBuff, U32 ulLen)
 * 功能：获取数据库主机名
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_mysql_password(S8 *pszBuff, U32 ulLen);

/**
 * 函数：U32 config_get_mysql_dbname(S8 *pszBuff, U32 ulLen)
 * 功能：获取数据库主机名
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_mysql_dbname(S8 *pszBuff, U32 ulLen);

/**
 * 函数：U32 config_init()
 * 功能： 初始化配置模块
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数初始化配置模块。配置文件的搜索路径顺序为：
 * 		1.系统配置文件目录
 * 		2."../etc/" 把当前目录看作bin目录，上层目录为当前服务根目录，在服务根目录下面查找
 */
U32 config_init();

/**
 * 函数：U32 config_deinit()
 * 功能： 销毁配置模块
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
U32 config_deinit();

#if INCLUDE_BH_SERVER

/**
 * 函数：config_hb_get_process_list
 * 功能：获取指定index的路径下的进程信息
 * 参数：
 * 		U32 ulIndex：xml中process的路径编号
 * 		S8 *pszName：输出参数，存放进程名的缓存
 * 		U32 ulNameLen：存放进程名缓存的长度
 * 		S8 *pszVersion：输出参数，存放进程版本号
 * 		U32 ulVerLen：存放进程版本号缓存的长度
 * 返回值：
 * 		成功返回0，失败返回－1
 */
S32 config_hb_get_process_list(U32 ulIndex, S8 *pszName, U32 ulNameLen, S8 *pszVersion, U32 ulVerLen);

/**
 * 函数：S32 config_hb_init()
 * 功能： 初始化心跳模块的配置
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数初始化配置模块。配置文件的搜索路径顺序为：
 * 		1.系统配置文件目录
 * 		2."../etc/" 把当前目录看作bin目录，上层目录为当前服务根目录，在服务根目录下面查找
 */
S32 config_hb_init();

/**
 * 函数：S32 config_hb_deinit()
 * 功能： 销毁心跳模块配置
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_deinit();

/**
 * 函数：S32 config_hb_save()
 * 功能：保存心跳模块配置文件
 * 参数：
 * 返回值：成功返回0，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_save();

#endif

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

