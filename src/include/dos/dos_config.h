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
 * 函数：U32 config_get_db_host(S8 *pszBuff, U32 ulLen);
 * 功能：获取数据库主机名
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_host(S8 *pszBuff, U32 ulLen);

/**
 * 函数：U32 config_get_db_port();
 * 功能：获取数据库端口号
 * 参数：
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_port();

/**
 * 函数：U32 config_get_db_user(S8 *pszBuff, U32 ulLen);
 * 功能：获取数据库主机名
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_user(S8 *pszBuff, U32 ulLen);


/**
 * 函数：U32 config_get_syssrc_db_user(S8 *pszBuff, U32 ulLen);
 * 功能：获取资源监控数据库主机名
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_syssrc_db_user(S8 *pszBuff, U32 ulLen);


/**
 * 函数：U32 config_get_db_password(S8 *pszBuff, U32 ulLen);
 * 功能：获取ccsys数据库密码
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_password(S8 *pszBuff, U32 ulLen);

/**
 * 函数：U32 config_get_syssrc_db_password(S8 *pszBuff, U32 ulLen);
 * 功能：获取资源监控数据库密码
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_syssrc_db_password(S8 *pszBuff, U32 ulLen);


/**
 * 函数：U32 config_get_db_dbname(S8 *pszBuff, U32 ulLen);
 * 功能：获取ccsys数据库名
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_db_dbname(S8 *pszBuff, U32 ulLen);

/**
 * 函数：U32 config_get_syssrc_db_dbname(S8 *pszBuff, U32 ulLen);
 * 功能：获取系统资源数据库名
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_syssrc_db_dbname(S8 *pszBuff, U32 ulLen);

/**
 * 函数：U32 config_get_py_path(S8 *pszBuff, U32 ulLen);
 * 功能：获取Python脚本路径
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_py_path(S8 *pszBuff, U32 ulLen);

/**
 * 函数：
 * U32 config_get_mysqlsock_path(S8 *pszBuff, U32 ulLen);
 * 功能：获取MySQL数据库sock文件路径
 * 参数：
 * 		S8 *pszBuff： 缓存
 * 		U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_mysqlsock_path(S8 *pszBuff, U32 ulLen);

/**
 * 函数：U32 config_get_syssrc_writeDB(S8 *pszBuff, U32 ulLen)
 * 功能：获取资源监控模块是否写数据库
 * 参数：
 *      S8 *pszBuff： 缓存
 *      U32 ulLen：缓存长度
 * 返回值：成功返回0.失败返回－1
 */
U32 config_get_syssrc_writeDB(S8 *pszBuff, U32 ulLen);

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

S32 config_save();

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

/**
 * 函数：S32 config_hb_get_process_cfg_cnt();
 * 功能：获取配置的进程个数
 * 参数：
 * 返回值：成功配置进程的个数，失败返回－1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_get_process_cfg_cnt();

/**
 * 函数：S32 config_hb_get_start_cmd(U32 ulIndex, S8 *pszCmd, U32 ulLen);
 * 功能：获得启动进程的命令
 * 参数：
 * 返回值：成功返回0，失败返回-1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_get_start_cmd(U32 ulIndex, S8 *pszCmd, U32 ulLen);



/**
 * 函数：S32 config_threshold_mem(S32* plMem);
 * 功能：获取内存占用率阀值
 * 参数：
 * 返回值：成功返回0，失败返回-1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_threshold_mem(U32* pulMem);


/**
 * 函数：S32 config_threshold_mem(S32* plMem);
 * 功能：获取CPU占用率的阀值
 * 参数：
 * 返回值：成功返回0，失败返回-1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_threshold_cpu(U32* pulAvg, U32* pul5sAvg, U32 *pul1minAvg, U32 *pul10minAvg);


/**
 * 函数：S32 config_threshold_disk(S32 *plPartition, S32* plDisk);
 * 功能：获取磁盘占用率的阀值
 * 参数：
 * 返回值：成功返回0，失败返回-1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_threshold_disk(U32 *pulPartition, U32* pulDisk);

/**
 * 函数：S32 config_hb_threshold_bandwidth(U32* pulBandWidth);
 * 功能：获取网络最大占用带宽
 * 参数：
 * 返回值：成功返回0，失败返回-1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_threshold_bandwidth(U32* pulBandWidth);

/**
 * 函数：S32 config_hb_threshold_proc(S32* plMem);
 * 功能：获取进程资源占用率的阀值
 * 参数：
 * 返回值：成功返回0，失败返回-1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
S32 config_hb_threshold_proc(U32* pulMem);

/**
 * 函数：U32 config_get_shortcut_cmd(U32 ulNo, S8 *pszCtrlCmd, U32 ulLen);
 * 功能：获取ctrl_panel模块快捷键支持的命令
 * 参数：
 * 返回值：成功返回0，失败返回-1
 *
 * 说明：该函数在销毁之前不会保存当前配置到文件，如果配置有更改，请提前保存
 */
U32 config_get_shortcut_cmd(U32 ulNo, S8 *pszCtrlCmd, U32 ulLen);

#endif //end INCLUDE_BH_SERVER

#ifdef INCLUDE_PTS

U32 config_get_pts_port1();
U32 config_get_pts_port2();
U32 config_get_web_server_port();
U32 config_get_pts_proxy_port();
U32 config_get_pts_telnet_server_port();
//U32 config_get_pts_domain(S8 *pszBuff, U32 ulLen);

#endif // end INCLUDE_PTS

#ifdef INCLUDE_PTC

U32 config_get_pts_major_domain(S8 *pszBuff, U32 ulLen);
U32 config_get_pts_minor_domain(S8 *pszBuff, U32 ulLen);
U32 config_get_pts_major_port();
U32 config_get_pts_minor_port();
U32 config_get_ptc_name(S8 *pszBuff, U32 ulLen);
U32 config_set_pts_major_domain(S8 *pszBuff);
U32 config_set_pts_minor_domain(S8 *pszBuff);
U32 config_set_pts_major_port(S8 *pszBuff);
U32 config_set_pts_minor_port(S8 *pszBuff);

#endif //end INCLUDE_PTC

#endif  //end INCLUDE_XML_CONFIG

#ifdef __cplusplus
}
#endif /* __cplusplus */

