
/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_sysstat.h
 *
 *  创建时间: 2015年6月29日17:28:16
 *  作    者: Larry
 *  描    述: 获取系统信息的文件
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#if (DOS_INCLUDE_SYS_STAT)

U32 dos_sysstat_cpu_start();
U32 dos_get_cpu_idel_percentage();

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

