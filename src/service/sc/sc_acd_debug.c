/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_acd_debug.c
 *
 *  创建时间: 2015年1月14日14:42:07
 *  作    者: Larry
 *  描    述: ACD模块相关功能函数实现
 *  修改历史:
 */

#include <dos.h>

U32  g_ulTraceLevel = 0;

VOID sc_acd_trace(U32 ulLevel, S8 *pszFormat, ...)
{

}

U32 sc_acd_set_trace_level(U32 ulLevel)
{
    g_ulTraceLevel = ulLevel;

    return DOS_SUCC;
}


