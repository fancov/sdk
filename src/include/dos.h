/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dosutil.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: DOS公共头文件
 *     History:
 */

#ifndef __DOSUTIL_H__
#define __DOSUTIL_H__

#include <env.h>

/* system header files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <asm/errno.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>
#include <netinet/in.h>
#include <semaphore.h>


#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_object_private.h>
#include <json-c/linkhash.h>

/* dos header files */
#include <dos/dos_types.h>
#include <syscfg.h>
#include <cfg/cfg_resource.h>
#include <dos/dos_config.h>
#include <dos/dos_task.h>
#include <dos/dos_def.h>
#include <dos/dos_mem.h>
#include <dos/dos_tmr.h>
#include <dos/dos_cli.h>
#include <dos/dos_log.h>
#include <dos/dos_hb.h>
#include <dos/dos_debug.h>
#include <dos/dos_string.h>
#include <dos/dos_db.h>
#include <list/list_pub.h>
#include <hash/hash.h>
#include <json/dos_json.h>

#if INCLUDE_LICENSE_CLIENT
#include <license.h>
#endif


#define DOS_VERSION  "1.0"

#endif

