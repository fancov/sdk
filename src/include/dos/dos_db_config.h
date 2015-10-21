#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifndef __DOS_DB_CONFIG_H__
#define __DOS_DB_CONFIG_H__

#if INCLUDE_DB_CONFIG

enum {
    LANG_TYPE_CN   = 0,
    LANG_TYPE_EN,
    LANG_TYPE_BUTT
}SC_LANG_TYPE;

enum {
    PARAM_LANG_TYPE                = 1,
    PARAM_WEB_PORT                 = 2,
    PARAM_AUDIO_KEY                = 3,
    PARAM_TOTAL_TIME               = 4,
    PARAM_SIP_REG_INTERVAL         = 5,
    PARAM_RTP_START_PORT           = 6,
    PARAM_RTP_END_PORT             = 7,
    PARAM_ENABLE_NAT               = 8,
    PARAM_SIP_CODEC                = 9,
    PARAM_RAS_ACCESS_IP            = 10,
    PARAM_RAS_ACCESS_PORT          = 11,
    PARAM_CALL_RATE                = 12,
    PARAM_RESTART_ONTIME           = 13,
    PARAM_RESTART_ONTIME_ENABLE    = 14,
    PARAM_RESTART_CYCLE            = 15,
    PARAM_RESTART_CYCLE_ENABLE     = 16,

    PARAM_BUTT,
};

/**
 * 函数: dos_db_config_get_param
 * 功能: 获取数据库参数
 * 参数:
 *       U32 ulIndex     : 参数编号
 *       S8 *pszBuffer   : 输出参数，存储参数内容
 *       U32 ulBufferLen : 输入参数，存储参数缓存长度
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * 该函数会实时从数据库中读取数据，如果频繁使用，请将参数缓存，以提高效率
 */
U32 dos_db_config_get_param(U32 ulIndex, S8 *pszBuffer, U32 ulBufferLen);

/**
 * 函数: dos_db_config_init
 * 功能: 初始化该模块
 * 参数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 *
 * 该函数会使用xml配置获取数据库配置文件，因此，请在xml配置文件初始化完成之后再调用该函数
 */
U32 dos_db_config_init();


#endif /* end of INCLUDE_DB_CONFIG */

#endif /* end of __DOS_DB_CONFIG_H__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */


