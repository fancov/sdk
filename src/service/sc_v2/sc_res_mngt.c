/**
 * @file : sc_res_mngt.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 各种资源管理
 *
 * @date: 2016年1月14日
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_def.h"
#include "sc_res.h"
#include "sc_pub.h"
#include "sc_debug.h"


/** SIP账户HASH表 REFER TO SC_USER_ID_NODE_ST */
HASH_TABLE_S             *g_pstHashSIPUserID  = NULL;
pthread_mutex_t          g_mutexHashSIPUserID = PTHREAD_MUTEX_INITIALIZER;

/** DID号码hash表 REFER TO SC_DID_NODE_ST */
HASH_TABLE_S             *g_pstHashDIDNum  = NULL;
pthread_mutex_t          g_mutexHashDIDNum = PTHREAD_MUTEX_INITIALIZER;

/* 路由数据链表 REFER TO SC_ROUTE_NODE_ST */
DLL_S                    g_stRouteList;
pthread_mutex_t          g_mutexRouteList = PTHREAD_MUTEX_INITIALIZER;

/* 企业链表 */
DLL_S                    g_stCustomerList;
pthread_mutex_t          g_mutexCustomerList = PTHREAD_MUTEX_INITIALIZER;

/* 网关列表 refer to SC_GW_NODE_ST (使用HASH) */
HASH_TABLE_S             *g_pstHashGW = NULL;
pthread_mutex_t          g_mutexHashGW = PTHREAD_MUTEX_INITIALIZER;

/* 网关组列表， refer to SC_GW_GRP_NODE_ST (使用HASH) */
HASH_TABLE_S             *g_pstHashGWGrp = NULL;
pthread_mutex_t          g_mutexHashGWGrp = PTHREAD_MUTEX_INITIALIZER;

/* 号码变换数据链表 REFER TO SC_NUM_TRANSFORM_NODE_ST */
DLL_S                    g_stNumTransformList;
pthread_mutex_t          g_mutexNumTransformList = PTHREAD_MUTEX_INITIALIZER;

/* 黑名单HASH表 */
HASH_TABLE_S             *g_pstHashBlackList  = NULL;
pthread_mutex_t          g_mutexHashBlackList = PTHREAD_MUTEX_INITIALIZER;

HASH_TABLE_S             *g_pstHashCaller = NULL;
pthread_mutex_t          g_mutexHashCaller = PTHREAD_MUTEX_INITIALIZER;

/* 主叫号码组列表，参照SC_CALLER_GRP_NODE_ST */
HASH_TABLE_S             *g_pstHashCallerGrp = NULL;
pthread_mutex_t          g_mutexHashCallerGrp = PTHREAD_MUTEX_INITIALIZER;

/* TT号和EIX对应关系表 */
HASH_TABLE_S             *g_pstHashTTNumber = NULL;
pthread_mutex_t          g_mutexHashTTNumber = PTHREAD_MUTEX_INITIALIZER;

/* 主叫号码限制hash表 refer to SC_NUMBER_LMT_NODE_ST */
HASH_TABLE_S             *g_pstHashNumberlmt = NULL;
pthread_mutex_t          g_mutexHashNumberlmt = PTHREAD_MUTEX_INITIALIZER;

/* 业务控制 */
HASH_TABLE_S             *g_pstHashServCtrl = NULL;
pthread_mutex_t          g_mutexHashServCtrl = PTHREAD_MUTEX_INITIALIZER;

/* 主叫号码设定列表，参照SC_CALLER_SETTING_ST */
HASH_TABLE_S             *g_pstHashCallerSetting = NULL;
pthread_mutex_t          g_mutexHashCallerSetting = PTHREAD_MUTEX_INITIALIZER;

U32 sc_res_init()
{
    DLL_Init(&g_stRouteList);
    DLL_Init(&g_stCustomerList);
    DLL_Init(&g_stNumTransformList);

    g_pstHashGWGrp = hash_create_table(SC_GW_GRP_HASH_SIZE, NULL);
    g_pstHashGW = hash_create_table(SC_GW_HASH_SIZE, NULL);
    g_pstHashDIDNum = hash_create_table(SC_DID_HASH_SIZE, NULL);
    g_pstHashSIPUserID = hash_create_table(SC_SIP_ACCOUNT_HASH_SIZE, NULL);
    g_pstHashBlackList = hash_create_table(SC_BLACK_LIST_HASH_SIZE, NULL);
    g_pstHashTTNumber = hash_create_table(SC_EIX_DEV_HASH_SIZE, NULL);
    g_pstHashCaller = hash_create_table(SC_CALLER_HASH_SIZE, NULL);
    g_pstHashCallerGrp = hash_create_table(SC_CALLER_GRP_HASH_SIZE, NULL);
    g_pstHashCallerSetting = hash_create_table(SC_CALLER_SETTING_HASH_SIZE, NULL);
    g_pstHashServCtrl = hash_create_table(SC_SERV_CTRL_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstHashGW)
        || DOS_ADDR_INVALID(g_pstHashGWGrp)
        || DOS_ADDR_INVALID(g_pstHashDIDNum)
        || DOS_ADDR_INVALID(g_pstHashSIPUserID)
        || DOS_ADDR_INVALID(g_pstHashBlackList)
        || DOS_ADDR_INVALID(g_pstHashCaller)
        || DOS_ADDR_INVALID(g_pstHashCallerGrp)
        || DOS_ADDR_INVALID(g_pstHashTTNumber)
        || DOS_ADDR_INVALID(g_pstHashServCtrl))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Alloc memort for res hash table fail");

        return DOS_FAIL;
    }

    /* 以下三项加载顺序不能更改 */
    sc_gateway_load(SC_INVALID_INDEX);
    sc_gateway_group_load(SC_INVALID_INDEX);
    sc_gateway_relationship_load();

    sc_route_load(SC_INVALID_INDEX);
    sc_transform_load(SC_INVALID_INDEX);
    sc_customer_load(SC_INVALID_INDEX);
    sc_did_load(SC_INVALID_INDEX);
    sc_sip_account_load(SC_INVALID_INDEX);
    sc_black_list_load(SC_INVALID_INDEX);
    sc_eix_dev_load(SC_INVALID_INDEX);

    /* 以下三项的加载顺序同样不可乱,同时必须保证之前已经加载DID号码(号码组业务逻辑) */
    sc_caller_load(SC_INVALID_INDEX);
    sc_caller_group_load(SC_INVALID_INDEX);
    sc_caller_relationship_load();

    sc_caller_setting_load(SC_INVALID_INDEX);
    sc_serv_ctrl_load(SC_INVALID_INDEX);

    return DOS_SUCC;
}

U32 sc_res_start()
{
    return DOS_SUCC;
}

U32 sc_res_stop()
{
    return DOS_SUCC;
}



#ifdef __cplusplus
}
#endif /* End of __cplusplus */


