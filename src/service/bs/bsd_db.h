/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名
 *
 *  创建时间:
 *  作    者:
 *  描    述:
 *  修改历史:
 */

#ifndef __BSD_DB_H__
#define __BSD_DB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifndef MAX_BUFF_LENGTH
#define MAX_BUFF_LENGTH 1024
#endif


/* 操作的表类型定义 */
enum BS_TABLE_TYPE_E
{
    BS_TBL_TYPE_CUSTOMER            = 0,        /* 客户表 */
    BS_TBL_TYPE_AGENT               = 1,        /* 坐席表 */
    BS_TBL_TYPE_BILLING_PACKAGE     = 2,        /* 资费表 */
    BS_TBL_TYPE_SETTLE              = 3,        /* 结算表 */
    BS_TBL_TYPE_TMP_CMD             = 4,        /* 由WEB发起的命令存的的临时表 */
    BS_TBL_TYPE_TASK                = 5,        /* 群呼任务 */

    BS_TBL_TYPE__BUTT               = 255
};


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif




