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

#ifndef __BS_DEF_H__
#define __BS_DEF_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#define BS_ONLY_LISTEN_LOCAL                DOS_FALSE/* 是否只监听本机 */
#define BS_LOCAL_IP_ADDR                    ((127<<24)|1)

#define BS_HASH_TBL_CUSTOMER_SIZE           (1<<9)      /* 客户信息HASH表大小 */
#define BS_HASH_TBL_AGENT_SIZE              (1<<12)     /* 坐席信息HASH表大小 */
#define BS_HASH_TBL_BILLING_PACKAGE_SIZE    (1<<8)      /* 计费包HASH表大小 */
#define BS_HASH_TBL_SETTLE_SIZE             (1<<6)      /* 结算HASH表大小 */
#define BS_HASH_TBL_TASK_SIZE               (1<<8)      /* 任务HASH表大小 */
#define BS_MAX_CUSTOMER_NAME_LEN            48          /* 客户名称最大长度 */
#define BS_MAX_PACKAGE_NAME_LEN             48          /* 资费包名称最大长度 */
#define BS_MAX_BILLING_RULE_IN_PACKAGE      10          /* 单个业务计费规则的最大条数 */
#define BS_TRACE_BUFF_LEN                   1024        /* buff字符串最大长度 */
#define BS_MAX_APP_CONN_NUM                 4           /* 应用层网络连接的最大数量 */
#define BS_MAX_MSG_LEN                      1024        /* 消息最大长度 */
#define BS_MAX_VOICE_SESSION_TIME           7200        /* 单次语音通话最大时间 */
#define BS_MAX_MESSAGE_NUM                  10000       /* 单次消息最大条数 */
#define BS_MASK_REGION_COUNTRY              0xFFFF0000  /* 国家代码(32-17bit),省编号(16-11bit),区号(10-1bit) */
#define BS_MASK_REGION_PROVINCE             0xFC00      /* 国家代码(32-17bit),省编号(16-11bit),区号(10-1bit) */
#define BS_MASK_REGION_CITY                 0x3FF       /* 国家代码(32-17bit),省编号(16-11bit),区号(10-1bit) */
#define BS_ACCOUNTING_INTERVAL              (2*60)      /* 出账时间间隔,单位:秒 */
#define BS_AUDIT_INTERVAL                   (10*60)     /* 审计时间间隔,单位:秒 */
#define BS_BACKUP_INTERVAL                  (60*60)     /* 备份时间间隔,单位:秒 */
#define BS_SYS_OPERATOR_ID                  0           /* 操作者为系统时的操作者ID */



/* 定义错误码 */
enum BS_INTER_ERRCODE_E
{
    BS_INTER_ERR_FAIL               = -1,       /* 失败 */
    BS_INTER_ERR_SUCC               = 0,        /* 成功 */
    BS_INTER_ERR_NOT_EXIST          = 1,        /* 不存在 */
    BS_INTER_ERR_MEM_ALLOC_FAIL     = 2,        /* 过期/失效 */

};

enum BS_WEB_CMD_OPTERATOR
{
    BS_CMD_INSERT,
    BS_CMD_UPDATE,
    BS_CMD_DELETE,

    BS_CMD_BUTT
};

/* 跟踪类型 */
enum BS_TRACE_TARGET_E
{
    BS_TRACE_FS                     = 1,            /* freeswitch消息 */
    BS_TRACE_WEB                    = (1<<1),       /* WEB消息 */
    BS_TRACE_DB                     = (1<<2),       /* DB */
    BS_TRACE_CDR                    = (1<<3),       /* CDR */
    BS_TRACE_BILLING                = (1<<4),       /* 计费 */
    BS_TRACE_ACCOUNT                = (1<<5),       /* 账户 */
    BS_TRACE_STAT                   = (1<<6),       /* 统计 */
    BS_TRACE_AUDIT                  = (1<<7),       /* 审计 */
    BS_TRACE_MAINTAIN               = (1<<8),       /* 维护 */
    BS_TRACE_RUN                    = (1<<9),       /* 运行 */

    BS_TRACE_ALL                    = 0xFFFFFFFF
};

/* 客户类型 */
enum BS_CUSTOMER_TYPE_E
{
    BS_CUSTOMER_TYPE_CONSUMER           = 0,        /* 消费者 */
    BS_CUSTOMER_TYPE_AGENT              = 1,        /* 代理商 */
    BS_CUSTOMER_TYPE_TOP                = 2,        /* 顶级客户(即系统所有者) */
    BS_CUSTOMER_TYPE_SP                 = 3,        /* 业务提供商 */

    BS_CUSTOMER_TYPE_BUTT               = 255
};

/* 客户状态 */
enum BS_CUSTOMER_STATE_E
{
    BS_CUSTOMER_STATE_ACTIVE            = 0,        /* 活动 */
    BS_CUSTOMER_STATE_FROZEN            = 1,        /* 冻结 */
    BS_CUSTOMER_STATE_INACTIVE          = 2,        /* 不可用 */

    BS_CUSTOMER_STATE_BUTT              = 255
};

/* 计费周期定义,0为无效值 */
enum BS_BILLING_CYCLE_E
{
    BS_BILLING_CYCLE_DAY                = 1,        /* 日 */
    BS_BILLING_CYCLE_WEEK               = 2,        /* 周 */
    BS_BILLING_CYCLE_MONTH              = 3,        /* 月 */
    BS_BILLING_CYCLE_YEAR               = 4,        /* 年 */

    BS_BILLING_CYCLE_BUTT               = 255
};

/* 运营商定义 */
enum BS_OPERATOR_E
{
    BS_OPERATOR_UNKNOWN                 = 0,        /* 未知运营商 */
    BS_OPERATOR_CHINANET                = 1,        /* 中国电信 */
    BS_OPERATOR_CHINAUNICOM             = 2,        /* 中国联通 */
    BS_OPERATOR_CMCC                    = 3,        /* 中国移动 */
    BS_OPERATOR_VNO                     = 4,        /* 虚拟运营商 */

    BS_OPERATOR_BUTT                    = 255
};

/* 号码类型,0为无效值 */
enum BS_NUMBER_TYPE_E
{
    BS_NUMBER_TYPE_MOBILE               = 1,        /* 国内移动电话号码 */
    BS_NUMBER_TYPE_FIXED                = 2,        /* 国内固定电话号码 */
    BS_NUMBER_TYPE_VOIP                 = 3,        /* VOIP号码(例如内部分机号码) */
    BS_NUMBER_TYPE_INTERNATIONAL        = 4,        /* 国际号码 */
    BS_NUMBER_TYPE_EMERGENCY            = 5,        /* 紧急呼叫 */
    BS_NUMBER_TYPE_SPECIAL              = 6,        /* 特殊号码(例如声讯台等) */

    BS_NUMBER_TYPE_BUTT                 = 255
};

/* 累计对象类型 */
enum BS_OBJECT_E
{
    BS_OBJECT_SYSTEM                    = 0,        /* 系统 */
    BS_OBJECT_CUSTOMER                  = 1,        /* 客户 */
    BS_OBJECT_ACCOUNT                   = 2,        /* 账户 */
    BS_OBJECT_AGENT                     = 3,        /* 坐席 */
    BS_OBJECT_NUMBER                    = 4,        /* 号码 */
    BS_OBJECT_LINE                      = 5,        /* 用户线(设备端口) */
    BS_OBJECT_TASK                      = 6,        /* 任务 */
    BS_OBJECT_TRUNK                     = 7,        /* 中继 */

    BS_OBJECT_BUTT                      = 255
};

/*
计费方式
暂不支持流量计费
*/
enum BS_BILLING_TYPE_E
{
    BS_BILLING_BY_TIMELEN               = 0,        /* 按时长计费 */
    BS_BILLING_BY_COUNT                 = 1,        /* 按次计费 */
    BS_BILLING_BY_TRAFFIC               = 2,        /* 按流量计费 */
    BS_BILLING_BY_CYCLE                 = 3,        /* 按周期计费 */

    BS_BILLING_TYPE_BUTT                = 255
};

/* 账户操作类型 */
enum enBS_ACCOUNT_OPERATE_TYPE_E
{
    BS_ACCOUNT_RECHARGE             = 1,        /* 充值 */
    BS_ACCOUNT_DEDUCTION            = 2,        /* 扣款 */
    BS_ACCOUNT_TRANSFER_IN          = 3,        /* 转入 */
    BS_ACCOUNT_TRANSFER_OUT         = 4,        /* 转出 */
    BS_ACCOUNT_REBATE_GET           = 5,        /* 获取返点 */
    BS_ACCOUNT_REBATE_PAY           = 6,        /* 支付返点 */

    BS_ACCOUNT_OPERATE_TYPE_BUTT    = 255
};

enum enBS_ACCOUNT_OPERATE_DIR_E
{
    BS_ACCOUNT_PAY                  = 1,
    BS_ACCOUNT_GET,

    BS_ACCOUNT_OPERATE_DIR_BUTT
};

/* 内部消息类型定义,0为无效值 */
enum BS_INTER_MSG_TYPE_E
{
    BS_INTER_MSG_WALK_REQ               = 1,        /* 表遍历请求 */
    BS_INTER_MSG_WALK_RSP               = 2,        /* 表遍历响应 */
    BS_INTER_MSG_ORIGINAL_CDR           = 3,        /* 原始话单 */
    BS_INTER_MSG_VOICE_CDR              = 4,        /* 语音话单 */
    BS_INTER_MSG_RECORDING_CDR          = 5,        /* 录音话单 */
    BS_INTER_MSG_MESSAGE_CDR            = 6,        /* 信息话单 */
    BS_INTER_MSG_SETTLE_CDR             = 7,        /* 结算话单 */
    BS_INTER_MSG_RENT_CDR               = 8,        /* 租金话单 */
    BS_INTER_MSG_ACCOUNT_CDR            = 9,        /* 账务话单 */
    BS_INTER_MSG_OUTBAND_STAT           = 10,       /* 呼出统计 */
    BS_INTER_MSG_INBAND_STAT            = 11,       /* 呼入统计 */
    BS_INTER_MSG_OUTDIALING_STAT        = 12,       /* 外呼统计 */
    BS_INTER_MSG_MESSAGE_STAT           = 13,       /* 消息统计 */
    BS_INTER_MSG_ACCOUNT_STAT           = 14,       /* 账务统计 */

    BS_INTER_MSG_BUTT                   = 255
};

/*
计费属性,0为无效值;连续编号,不超过31个;
用法说明:
1.归属地属性,32bit定义:3个字段,国家代码(32-17bit),省编号(16-11bit),区号(10-1bit)
2.归属地属性,匹配规则:
  a.0-通配所有;
  b.字段值:0-通配字段代表的所有地区;
  c.完全匹配:所有字段均相同;
3.运营商属性,匹配规则:
  a.0-通配所有;
  b.要么通配,要么匹配单个运营商;
4.号码类型属性,匹配规则:
  a.0-通配所有;
  b.要么通配,要么匹配单个号码类型;
5.时间属性,匹配规则:
  a.0-不限定时间;
  b.根据属性值,查询数据库,判断时间是否在数据库的时间定义中;
6.资源类属性,只有在租金类业务下使用,由系统自行计算处理,无需匹配.
  a.源属性1填写资源类属性;
  b.源属性值1为资源数量,源属性值2为计费周期,费率为资源单价;
  c.源属性值1为0表示由系统统计数量;
  d.目的属性及属性值,以后根据各类资源再行定义;
*/
enum BS_BILLING_ATTRIBUTE_E
{
    BS_BILLING_ATTR_REGION              = 1,        /* 归属地 */
    BS_BILLING_ATTR_TEL_OPERATOR        = 2,        /* 运营商 */
    BS_BILLING_ATTR_NUMBER_TYPE         = 3,        /* 号码类型 */
    BS_BILLING_ATTR_TIME                = 4,        /* 时间类型 */
    BS_BILLING_ATTR_ACCUMULATE_TIMELEN  = 5,        /* 累计时长 */
    BS_BILLING_ATTR_ACCUMULATE_COUNT    = 6,        /* 累计次数 */
    BS_BILLING_ATTR_ACCUMULATE_TRAFFIC  = 7,        /* 累计流量 */
    BS_BILLING_ATTR_CONCURRENT          = 8,        /* 并发数 */
    BS_BILLING_ATTR_SET                 = 9,        /* 套餐 */
    BS_BILLING_ATTR_RESOURCE_AGENT      = 10,       /* 坐席资源 */
    BS_BILLING_ATTR_RESOURCE_NUMBER     = 11,       /* 号码资源 */
    BS_BILLING_ATTR_RESOURCE_LINE       = 12,       /* 用户线(设备端口)资源 */

    BS_BILLING_ATTR_LAST                = 13,       /* 特殊标记,最后一个有效属性 */

    BS_BILLING_ATTR_BUTT                = 31
};

/*
累计规则
特别的,业务类型集通过1左移业务类型值再取或而来,意味着目前只支持31个以内的业务类型
*/
typedef struct stBS_ACCUMULATE_RULE
{
    U32     ulRuleID;                               /* 规则ID */
    U8      ucObject;                               /* 累计对象,参考:BS_OBJECT_E */
    U8      ucCycle;                                /* 累计周期,参考:BS_BILLING_CYCLE_E */
    U32     ulServTypeSet;                          /* 业务类型集 */
    U32     ulMaxValue;                             /* 最大值 */
}BS_ACCUMULATE_RULE;

/*
计费规则匹配结构体
注意:
1.对于最终消费者,客户ID与消费者客户ID相同;
2.对于代理商,匹配累计类属性时,依然使用最终消费者的客户ID进行匹配;
*/
typedef struct stBS_BILLING_MATCH_ST
{
    BS_ACCUMULATE_RULE  stAccuRule;

    U32     ulCustomerID;                           /* 客户ID */
    U32     ulComsumerID;                           /* 消费者客户ID */
    U32     ulAgentID;
    U32     ulUserID;
    U32     ulTimeStamp;
    S8      szCaller[BS_MAX_CALL_NUM_LEN];
    S8      szCallee[BS_MAX_CALL_NUM_LEN];

}BS_BILLING_MATCH_ST;


/*
计费规则结构体
用法说明:
1.一个计费package包含多个业务的计费规则,一个业务包含多条不同优先级的计费规则;
2.计费处理时,按照优先级从高到低匹配,匹配成功则计费结束;
3.对于优惠费率设置,可通过设置高优先级来实现;
4.属性类型为255时,表示不限定属性;
5.套餐月租通过设定另外一条周期计费规则实现即可;
6.套餐类费率设置时,可设定累计属性并设定费率为0;
7.最低消费类配置,可通过设置周期计费类型来实现(等价于套餐内免费,套餐外收费);
8.当设定为累计属性,且属性值为0时,意味着总数不限;
9.属性为归属地时,32bit属性值字段定义:国家代码(32-17bit),省编号(16-11bit),区号(10-1bit)
10.归属地属性值可以通配,例如区号为0时意味着通配符合国家代码及省编码下所有的市区;
11.属性为时长时,属性值单位为秒;属性为流量时,属性值单位为KB;
12.号码类属性,属性值为0表示不限制;否则,对应的号码,需要在数据库中查找匹配;
13.时间类属性,属性值表示一个设定的时间范围,需要到数据库中匹配查找(分时段优惠可能使用);
14.周期计费规则,通过周期的计费处理完成,不是由话单计费触发;
15.资源类扣费规则(例如坐席月租),业务类型填租金类业务,源属性填资源类别,
   属性值为资源数量,填0表示由系统统计数量,费率为资源单价;其它字段可以不填;
16.按次计费规则,所有计费单位须为1,计费单位内计费次数也须为1;
17.单次费率为U32_BUTT,表示需要根据被叫号码单独查询费率,主要用于国际长途呼叫;暂未实现;
设计实现:
1.内存中以PackageID+业务类型为索引创建哈希表,将所有的计费规则读到内存中;
2.同一PackageID+业务类型的所有计费规则,存放到结构体数组中;
3.数据库数据的维护更新操作,需要实时同步到内存中;
*/
typedef struct stBS_BILLING_RULE_ST
{
    U32     ulPackageID;                    /* 所属计费packageID */
    U32     ulRuleID;                       /* 规则ID,数据库中全局统一编号 */
    U8      ucSrcAttrType1;                 /* 源属性类型 */
    U8      ucSrcAttrType2;                 /* 源属性类型 */
    U8      ucDstAttrType1;                 /* 目的属性类型 */
    U8      ucDstAttrType2;                 /* 目的属性类型 */
    U32     ulSrcAttrValue1;                /* 源属性数值 */
    U32     ulSrcAttrValue2;                /* 源属性数值 */
    U32     ulDstAttrValue1;                /* 目的属性数值 */
    U32     ulDstAttrValue2;                /* 目的属性数值 */
    U32     ulFirstBillingUnit;             /* 首次计费单位,单位:秒或次 */
    U32     ulNextBillingUnit;              /* 后续计费单位,单位:秒或次 */
    U8      ucFirstBillingCnt;              /* 首次计费单位内计费次数 */
    U8      ucNextBillingCnt;               /* 后续计费单位内计费次数 */
    U8      ucServType;                     /* 业务类型,参考:BS_SERV_TYPE_E */
    U8      ucBillingType;                  /* 计费方式,参考:BS_BILLING_TYPE_E */
    U32     ulBillingRate;                  /* 单次费率,单位:1/100分,毫 */
    U32     ulEffectTimestamp;              /* 生效时间戳,0为立刻生效 */
    U32     ulExpireTimestamp;              /* 失效时间戳,0为永久有效 */
    U8      ucPriority;                     /* 优先级,0-9,越小优先级越高 */

    U8      ucValid;
    U8      aucReserv[2];

}BS_BILLING_RULE_ST;


/*
资费包结构体
设计实现:
1.所有的资费信息读取到内存中;
2.所有的资费信息,挂载在哈希表的节点中;
3.不同的业务类型,计费规则存放到哈希桶中;
4.数据库数据的维护更新操作,需要实时同步到内存中;
*/
typedef struct stBS_BILLING_PACKAGE_ST
{
    U32     ulPackageID;                    /* 资费包ID */
    U8      ucServType;                     /* 业务类型,参考:BS_SERV_TYPE_E */
    U8      aucReserv[3];

    BS_BILLING_RULE_ST  astRule[BS_MAX_BILLING_RULE_IN_PACKAGE];        /* 计费规则 */

}BS_BILLING_PACKAGE_ST;

/* 账户信息结构体 */
typedef struct stBS_ACCOUNT_ST
{
    pthread_mutex_t     mutexAccount;       /* 账户锁 */

    BS_STAT_ACCOUNT_ST  stStat;             /* 统计信息 */

    U32     ulAccountID;                    /* 账户ID */
    S8      szPwd[BS_MAX_PASSWORD_LEN];     /* 密码 */
    U32     ulCustomerID;                   /* 关联客户ID */
    S32     lCreditLine;                    /* 信用额度;单位:1/100分 */
    U32     ulBillingPackageID;             /* 关联计费packageID */
    S64     LBalance;                       /* 账户余额,单位:1/100分,毫;注意账户余额越界的情况 */
    S32     lBalanceWarning;                /* 告警阈值;单位:1/100分 */
    S32     lRebate;                        /* 未出账返点,单位:1/100分,毫 */
    S64     LBalanceActive;                 /* 动态账户余额,用于实时扣费尚未出账的临时状态;单位:1/100分 */
    U32     ulAccountingTime;               /* 出账时间 */
    U32     ulExpiryTime;                   /* 失效时间 */

}BS_ACCOUNT_ST;


/*
客户信息结构体
设计实现:
1.所有的客户信息读取到内存中(相应的,账户信息也存放到内存中);
2.所有的客户信息,挂载在哈希表的节点中;
3.建立客户树,子客户同级别的节点使用链表连接;
4.树链表节点中记录该客户在哈希表中的位置;
5.数据库数据的维护更新操作,需要实时同步到内存中;
*/
typedef struct stBS_CUSTOMER_ST
{
    DLL_NODE_S          stNode;             /* 链表节点,同一父客户的同级客户在同一链表中
                                               pHandle记录hash表节点地址;要求此字段为结构体第一个 */
    DLL_S               stChildrenList;     /* 子节点链表 */
    struct stBS_CUSTOMER_ST *pstParent;     /* 父节点 */

    BS_ACCOUNT_ST       stAccount;          /* 关联账户ID,目前只考虑1个客户对应1个账户的情况 */
    BS_STAT_SERV_ST     stStat;             /* 统计信息 */

    U32     ulCustomerID;                   /* 客户ID */
    S8      szCustomerName[BS_MAX_CUSTOMER_NAME_LEN];   /* 客户名称 */
    U32     ulParentID;                     /* 父客户ID */

    U32     ulAgentNum;                     /* 坐席数量 */
    U32     ulNumberNum;                    /* 号码(长号)数量 */
    U32     ulUserLineNum;                  /* 用户线数量 */

    U8      ucCustomerType;                 /* 客户类型 */
    U8      ucCustomerState;                /* 客户状态 */

    U8      aucReserv[2];

}BS_CUSTOMER_ST;
/*
坐席信息结构体
设计实现:
1.所有的坐席信息读取到内存中
2.内存中的坐席数据信息使用使用hash表来存储;
3.数据库数据的维护更新操作,需要实时同步到内存中;
*/
typedef struct stBS_AGENT_ST
{
    BS_STAT_SERV_ST     stStat;             /* 统计信息 */

    U32     ulAgentID;                      /* 坐席ID,要求全数字,不超过10位,最高位小于4 */
    U32     ulCustomerID;                   /* 客户ID */
    U32     ulGroup1;                       /* 所属班组1 */
    U32     ulGroup2;                       /* 所属班组2 */

}BS_AGENT_ST;

/*
结算信息结构体
设计实现:
1.所有的结算信息读取到内存中
2.内存中的结算数据信息使用使用hash表来存储;
3.数据库数据的维护更新操作,需要实时同步到内存中;
*/
typedef struct stBS_SETTLE_ST
{
    BS_STAT_SERV_ST     stStat;             /* 统计信息 */

    U32     ulSPID;                         /* 服务提供商ID,要求全数字,不超过10位,最高位小于4 */
    U32     ulPackageID;                    /* 所属计费packageID */
    U16     usTrunkID;                      /* 中继ID */
    U8      ucSettleFlag;                   /* 结算标志,1:结算,0:不结算 */

    U8      ucReserv;

}BS_SETTLE_ST;

/*
 * WEBAPI数据结构体
 **/
typedef struct tagBSWEBCMDInfo
{
    U32         ulTblIndex;                     /* 被更新的表 */
    U32         ulTimestamp;                    /* 当前数据创建的时间 */
    JSON_OBJ_ST *pstData;                   /* JSON对象 */
}BS_WEB_CMD_INFO_ST;

/*
任务信息结构体
设计实现:
1.主要用于记录任务的统计信息;
2.根据识别到的任务生成哈希表节点存储;
3.只记录当天任务的统计信息,隔日全部重新清0;
*/
typedef struct stBS_TASK_ST
{
    BS_STAT_SERV_ST     stStat;             /* 统计信息 */

    U32     ulTaskID;                       /* 任务ID,要求全数字,不超过10位,最高位小于4 */
    U8      aucReserv[3];

}BS_TASK_ST;

/* 数据层与业务层之间的消息结构体 */
typedef struct stBS_INTER_MSG_TAG
{
    pthread_t   srcTid;
    pthread_t   dstTid;
    U8      ucMsgType;                      /* 消息类型 */
    U8      ucReserv;
    U16     usMsgLen;                       /* 消息总长度 */
    S32     lInterErr;                      /* 内部错误码 */
}BS_INTER_MSG_TAG;

typedef S32 (*BS_FN)(U32, VOID *);
/* 表项遍历消息结构体 */
typedef struct stBS_INTER_MSG_WALK
{
    BS_INTER_MSG_TAG    stMsgTag;

    U32     ulTblType;                      /* 表类型 */
    BS_FN   FnCallback;                     /* 数据库操作的回调函数 */
    VOID    *param;                         /* 回调函数参数 */
}BS_INTER_MSG_WALK;

/* 话单消息结构体 */
typedef struct stBS_INTER_MSG_CDR
{
    BS_INTER_MSG_TAG    stMsgTag;

    VOID    *pCDR;                          /* cdr结构体指针 */
}BS_INTER_MSG_CDR;

/* 统计消息结构体 */
typedef struct stBS_INTER_MSG_STAT
{
    BS_INTER_MSG_TAG    stMsgTag;

    VOID    *pStat;                         /*  统计消息结构体指针 */
}BS_INTER_MSG_STAT;




typedef struct stBSS_APP_CONN
{
    U32                 bIsValid:1;
    U32                 bIsConn:1;
    U32                 ucCommType:8;
    U32                 ulReserv:22;

    U32                 ulMsgSeq;

    U32                 aulIPAddr[4];       /* APP所在IP地址,网络序 */
    S32                 lSockfd;            /* APP对应在本机的socket */
    S32                 lAddrLen;
    union tagSocketAddr{
        struct sockaddr_in  stInAddr;             /* APP对应在本机的连接地址 */
        struct sockaddr_un  stUnAddr;             /* APP对应在本机的连接地址 */
    }stAddr;
}BSS_APP_CONN;

/* 业务层控制块 */
typedef struct stBSS_CB
{
    U32         bIsMaintain:1;              /* 是否系统维护中 */
    U32         bIsBillDay:1;               /* 是否账单日 */
    U32         bIsDayCycle:1;              /* 日循环超时 */
    U32         bIsHourCycle:1;             /* 每小时循环超时 */
    U32         ulHour:5;                   /* 当前小时数 */
    U32         ulTraceLevel:3;             /* 跟踪级别 */
    U32         ulReserv:20;
    U32         ulTraceFlag;                /* 跟踪标志 */

    U32         ulCDRMark;                  /* CDR标记,用于原始话单拆分时标记同一来源 */
    U32         ulMaxVocTime;               /* 系统级单次语音最大时长,单位:秒 */
    U32         ulMaxMsNum;                 /* 系统级单次消息最大条数 */
    U32         ulStatDayBase;              /* 按天统计基础时间戳 */
    U32         ulStatHourBase;             /* 按时统计基础时间戳 */

    U32         ulCommProto;                /* 使用哪种协议通讯 */
    U16         usUDPListenPort;            /* 监听端口,网络序 */
    U16         usTCPListenPort;            /* 监听端口,网络序 */

    BSS_APP_CONN    astAPPConn[BS_MAX_APP_CONN_NUM];
    BS_CUSTOMER_ST  *pstTopCustomer;        /* 顶级客户信息 */

    DOS_TMR_ST  hDayCycleTmr;               /* 循环定时器 */
    DOS_TMR_ST  hHourCycleTmr;              /* 循环定时器 */
}BSS_CB;

extern pthread_mutex_t g_mutexTableUpdate;
extern BOOL                 g_bTableUpdate;

extern pthread_mutex_t  g_mutexCustomerTbl;
extern HASH_TABLE_S     *g_astCustomerTbl;
extern pthread_mutex_t  g_mutexBillingPackageTbl;
extern HASH_TABLE_S     *g_astBillingPackageTbl;
extern pthread_mutex_t  g_mutexSettleTbl;
extern HASH_TABLE_S     *g_astSettleTbl;
extern pthread_mutex_t  g_mutexAgentTbl;
extern HASH_TABLE_S     *g_astAgentTbl;
extern pthread_mutex_t  g_mutexTaskTbl;
extern HASH_TABLE_S     *g_astTaskTbl;

extern pthread_cond_t   g_condBSS2DList;
extern pthread_mutex_t  g_mutexBSS2DMsg;
extern DLL_S            g_stBSS2DMsgList;
extern pthread_cond_t   g_condBSD2SList;
extern pthread_mutex_t  g_mutexBSD2SMsg;
extern DLL_S            g_stBSD2SMsgList;
extern pthread_cond_t   g_condBSAppSendList;
extern pthread_mutex_t  g_mutexBSAppMsgSend;
extern DLL_S            g_stBSAppMsgSendList;
extern pthread_cond_t   g_condBSAAAList;
extern pthread_mutex_t  g_mutexBSAAAMsg;
extern DLL_S            g_stBSAAAMsgList;
extern pthread_cond_t   g_condBSCDRList;
extern pthread_mutex_t  g_mutexBSCDR;
extern DLL_S            g_stBSCDRList;
extern pthread_cond_t   g_condBSBillingList;
extern pthread_mutex_t  g_mutexBSBilling;
extern DLL_S            g_stBSBillingList;
extern pthread_mutex_t  g_mutexWebCMDTbl;
extern DLL_S            g_stWebCMDTbl;
extern U32              g_ulLastCMDTimestamp;


extern BSS_CB g_stBssCB;

/* bs_ini.c */
VOID bs_init_customer_st(BS_CUSTOMER_ST *pstCustomer);
VOID bs_init_agent_st(BS_AGENT_ST *pstAgent);
VOID bs_init_billing_package_st(BS_BILLING_PACKAGE_ST *pstBillingPackage);
VOID bs_init_settle_st(BS_SETTLE_ST *pstSettle);
VOID bs_build_customer_tree(VOID);


/* bs_lib.c */
VOID bs_free_node(VOID *pNode);
S32 bs_customer_hash_node_match(VOID *pKey, HASH_NODE_S *pNode);
S32 bs_agent_hash_node_match(VOID *pKey, HASH_NODE_S *pNode);
S32 bs_billing_package_hash_node_match(VOID *pKey, HASH_NODE_S *pNode);
S32 bs_settle_hash_node_match(VOID *pKey, HASH_NODE_S *pNode);
S32 bs_task_hash_node_match(VOID *pKey, HASH_NODE_S *pNode);
U32 bs_hash_get_index(U32 ulHashTblSize, U32 ulID);
HASH_NODE_S *bs_get_customer_node(U32 ulCustomerID);
BS_CUSTOMER_ST *bs_get_customer_st(U32 ulCustomerID);
BS_AGENT_ST *bs_get_agent_st(U32 ulAgentID);
BS_SETTLE_ST *bs_get_settle_st(U16 usTrunkID);
BS_TASK_ST *bs_get_task_st(U32 ulTaskID);
VOID bs_customer_add_child(BS_CUSTOMER_ST *pstCustomer, BS_CUSTOMER_ST *pstChildCustomer);
BSS_APP_CONN *bs_get_app_conn(BS_MSG_TAG *pstMsgTag);
BSS_APP_CONN *bs_save_app_conn(S32 lSockfd, U8 *pstAddrIn, U32 ulAddrinHeader, S32 lAddrLen);
VOID bs_stat_agent_num(VOID);
U32 bs_get_settle_packageid(U16 usTrunkID);
BS_BILLING_PACKAGE_ST *bs_get_billing_package(U32 ulPackageID, U8 ucServType);
BOOL bs_billing_rule_is_properly(BS_BILLING_RULE_ST  *pstRule);
U32 bs_pre_billing(BS_CUSTOMER_ST *pstCustomer, BS_MSG_AUTH *pstMsg, BS_BILLING_PACKAGE_ST *pstPackage);
BOOL bs_cause_is_busy(U16 usCause);
BOOL bs_cause_is_not_exist(U16 usCause);
BOOL bs_cause_is_no_answer(U16 usCause);
BOOL bs_cause_is_reject(U16 usCause);
VOID bs_account_stat_refresh(BS_ACCOUNT_ST *pstAccount, S32 lFee, S32 lPorfit, U8 ucServType);
VOID bs_stat_voice(BS_CDR_VOICE_ST *pstCDR);
VOID bs_stat_message(BS_CDR_MS_ST *pstCDR);
BS_BILLING_RULE_ST *bs_match_billing_rule(BS_BILLING_PACKAGE_ST *pstPackage,
                                              BS_BILLING_MATCH_ST *pstBillingMatch);

/* bs_msg.c */
VOID bsd_send_walk_rsp2sl(DLL_NODE_S *pMsgNode, S32 lReqRet);
VOID bsd_inherit_rule_req2sl(DLL_NODE_S *pMsgNode);
VOID bss_send_walk_req2dl(U32 ulTblType, BS_FN callback, VOID *param);
VOID bss_send_cdr2dl(DLL_NODE_S *pMsgNode, U8 ucMsgType);
VOID bss_send_stat2dl(DLL_NODE_S *pMsgNode, U8 ucMsgType);
VOID bss_send_rsp_msg2app(BS_MSG_TAG *pstMsgTag, U8 ucMsgType);
VOID bss_send_aaa_rsp2app(DLL_NODE_S *pMsgNode);

/* bs_debug.c */
VOID bs_trace(U32 ulTraceTarget, U8 ucTraceLevel, const S8 * szFormat, ...);
S32  bs_command_proc(U32 ulIndex, S32 argc, S8 **argv);
S32  bs_update_test(U32 ulIndex, S32 argc, S8 **argv);



/* bsd_mngt.c */
VOID *bsd_recv_bss_msg(VOID * arg);
VOID *bsd_backup(VOID *arg);

/* bss_mngt.c */
VOID *bss_recv_bsd_msg(VOID * arg);
VOID *bss_send_msg2app(VOID *arg);
VOID *bss_recv_msg_from_app(VOID *arg);
VOID *bss_recv_msg_from_web(VOID *arg);
VOID *bss_web_msg_proc(VOID *arg);
VOID *bss_aaa(VOID *arg);
VOID *bss_cdr(VOID *arg);
VOID *bss_billing(VOID *arg);
VOID *bss_stat(VOID *arg);
VOID *bss_accounting(VOID *arg);
VOID *bss_audit(VOID *arg);
VOID bss_day_cycle_proc(U64 uLParam);
VOID bss_hour_cycle_proc(U64 uLParam);

/* bsd_db.c */
S32 bs_init_db();
S32 bsd_walk_customer_tbl(BS_INTER_MSG_WALK *pstMsg);
S32 bsd_walk_agent_tbl(BS_INTER_MSG_WALK *pstMsg);
S32 bsd_walk_billing_package_tbl(BS_INTER_MSG_WALK *pstMsg);
S32 bsd_walk_billing_package_tbl_bak(U32 ulPkgID);
S32 bsd_walk_settle_tbl(BS_INTER_MSG_WALK *pstMsg);
S32 bsd_walk_web_cmd_tbl(BS_INTER_MSG_WALK *pstMsg);
S32 bsd_delete_web_cmd_tbl(BS_INTER_MSG_WALK *pstMsg);
VOID bsd_save_original_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_voice_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_recording_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_message_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_settle_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_rent_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_account_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_outband_stat(BS_INTER_MSG_STAT *pstMsg);
VOID bsd_save_inband_stat(BS_INTER_MSG_STAT *pstMsg);
VOID bsd_save_outdialing_stat(BS_INTER_MSG_STAT *pstMsg);
VOID bsd_save_message_stat(BS_INTER_MSG_STAT *pstMsg);
VOID bsd_save_account_stat(BS_INTER_MSG_STAT *pstMsg);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif



