/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  cfg_resource.h
 *
 *  Created on: 2014-11-17
 *      Author: Larry
 *        Todo: 配置进程中相关资源的个数
 *     History:
 */

/**
 * 定义每一个进程中最大定时器个数
 */
#define DOS_TIMER_MAX_NUM                1024

/**
 * 定义cli server 和 监控进程中最大链接的进程数
 */
#define DOS_PROCESS_MAX_NUM              32

/**
 * 定义telnet server最大客户端连接数
 */
#define DOS_CLIENT_MAX_NUM               8

/**
 * 定义最大进程名长度
 */
#define DOS_MAX_PROCESS_NAME_LEN 48

/**
 * 定义最大进程版本好长度
 */
#define DOS_MAX_PROCESS_VER_LEN  48

/**
 * 定义telnet server的监听端口
 */
#define DOS_TELNETD_LINSTEN_PORT         2801

/**
 * 定义内存管理hash表大小
 */
#define DOS_MEM_MNGT_HASH_SIZE           128

/**
 * 定义pts telnet server的监听端口
 */
#define  DOS_PTS_TELNETD_LINSTEN_PORT   28200

/**
 * 定义pts goahead的监听端口
 */
#define  DOS_PTS_WEB_SERVER_PORT        8080

/**
 * 定义pts web server的监听端口
 */
#define  DOS_PTS_PROXY_PORT             28000

/**
 * 定义pts server的监听端口
 */
#define  DOS_PTS_SERVER_PORT            28100