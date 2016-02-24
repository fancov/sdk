
#ifndef __LICENSE_H__
#define __LICENSE_H__

/*
 * 函数: U32 licc_init();
 * 功能: 初始化license客户端
 * 参数:
 * 返回值: 成返回DOS_SUCC，失败返回DOS_FAIL
 *   !!!!!!如果返回失败建议终止应用程序执行
 **/
extern U32 licc_init();

/*
 * 函数: licc_get_limitation
 * 功能: 获取LICENSE对某个模块的限制
 * 参数:
 *     U32 ulModIndex      : 模块编号
 *     U32 *pulLimitation  : 输出参数，对模块的限制
 * 返回值: 成返回DOS_SUCC，失败返回DOS_FAIL
 **/
extern U32 licc_get_limitation(U32 ulModIndex, U32 *pulLimitation);

/*
 * 函数: licc_get_default_limitation
 * 功能: 获取LICENSE对某个模块的默认限制
 * 参数:
 *     U32 ulModIndex      : 模块编号
 *     U32 *pulLimitation  : 输出参数，对模块的默认限制
 * 返回值: 成返回DOS_SUCC，失败返回DOS_FAIL
 **/
extern U32 licc_get_default_limitation(U32 ulModIndex, U32 *pulLimitation);

/*
 * 函数: licc_get_expire_check
 * 功能: 获取LICENSE对某个模块的时间限制
 * 参数:
 *     U32 ulModIndex      : 模块编号
 * 返回值: 如果License没有过期，返回DOS_SUCC，否则返回DOS_FAIL
 **/
extern U32 licc_get_expire_check(U32 ulModIndex);

/*
 * 函数: licc_get_machine
 * 功能: 获取机器码
 * 参数:
 *     S8 *pszMachine, 输出参数，传出机器码
 *     S32 *plLength, 输入输出参数, 传入缓存最大长度，传出机器码长度
 * 返回值: 成返回DOS_SUCC，失败返回DOS_FAIL
 **/
extern U32 licc_get_machine(S8 *pszMachine, S32 *plLength);

/*
 * 函数: lic_get_licc_version
 * 功能: 获取license客户端版本号
 * 参数:
 *     U32 *pulVersion, 输出参数，存储转换为整数的版本号
 * 返回值: 成返回DOS_SUCC，失败返回DOS_FAIL
 **/
U32 lic_get_licc_version(U32 *pulVersion);

/*
 * 函数: lic_get_licc_version_str
 * 功能: 获取license客户端版本号
 * 参数:
 * 返回值: 成返回字符串形式的版本号
 **/
S8* lic_get_licc_version_str();

/*
 * 函数: licc_get_license_loaded
 * 功能: 查询license是否被加载
 * 参数:
 * 返回值: 如果license已经被正常加载将返回DOS_TRIE,否则返回DOS_FALSE
 **/
BOOL licc_get_license_loaded();

/*
 * 函数: licc_save_customer_id
 * 功能: 保存客户信息
 * 参数:
 *     U8 *aucData  : 数据缓存
 *     U32 ulLength : 数据长度
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
U32 licc_save_customer_id(U8 *aucData, U32 ulLength);

/*
 * 函数: licc_load_customer_id
 * 功能: 保存客户信息
 * 参数:
 *     U8 *aucData  : 数据缓存
 *     U32 ulLength : 数据长度
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
U32 licc_load_customer_id(U8 *aucData, U32 ulLength);

/*
 * 函数: licc_save_license
 * 功能: 保存license信息
 * 参数:
 *     U8 *aucData  : 数据缓存
 *     U32 ulLength : 数据长度
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
U32 licc_save_license(U8 *auclicense, U32 ulLength);

/*
 * 函数: licc_get_license_version
 * 功能: 获取license版本号
 * 参数:
 *     U32 *pulVersion: license版本缓存
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
U32 licc_get_license_version(U32 *pulVersion);

/*
 * 函数: licc_get_license_expire
 * 功能: 获取license版本号
 * 参数:
 *     U32 *pulExpire: license版本缓存
 * 返回值:
 *     !!!!!!!!!!!!!
 *     请检查获取到的时间，如果为U32_BUTT，则是错误了，如果为0，则说明没有限制时间，如果其他值，则表示还可以使用多少秒钟
 **/
U32 licc_get_license_expire(U32 *pulExpire);

/*
 * 函数: licc_get_license_validity
 * 功能: 获取license可以使用的时间
 * 参数:
 *     U32 *pulValidity : 存储license总的有效期时长
 *     U32 *pulTimeRemaining : license剩余时长，如果过期将被设置为0
 * 返回值: 如果License没有过期，返回DOS_SUCC，否则返回DOS_FAIL
 * 注意:
 *      U32 *pulValidity, U32 *pulTimeRemaining 两项参数都可以为空
 **/
extern U32 licc_get_license_validity(U32 *pulValidity, U32 *pulTimeRemaining);

/*
 * 函数: licc_get_license_mod_validity
 * 功能: 获取license中特定模块可以使用的时间
 * 参数:
 *     U32 ulModIndex   : 模块编号
 *     U32 *pulValidity : 存储时间
 *     U32 *pulTimeRemaining : 剩余时间，如果过期将被设置为0
 * 返回值: 如果License没有过期，返回DOS_SUCC，否则返回DOS_FAIL
 * 注意:
 *      U32 *pulValidity, U32 *pulTimeRemaining 两项参数都可以为空
 **/
extern U32 licc_get_license_mod_validity(U32 ulModIndex, U32 *pulValidity, U32 *pulTimeRemaining);

/*
 * 函数: licc_set_srv_stat
 * 功能: 设置ulModuleID所指定的模块的的统计值
 * 参数:
 *     U32 ulModuleID:  模块ID
 *     U32 ulStat:      统计值
 * 返回值:
 *     成功返回SUCC，是否反正FAIL
 **/
U32 licc_set_srv_stat(U32 ulModuleID, U32 ulStat);

#endif /* __LICENSE_H__ */

