
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



#endif /* __LICENSE_H__ */

