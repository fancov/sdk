/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名: sc_ep_sb_adapter.c
 *
 *  创建时间: 2015年1月9日11:13:01
 *  作    者: Larry
 *  描    述: SC模块公共库函数
 *  修改历史:
 */

U32 sc_get_trunk_id(S8 *pszIPAddr, S8 *pszGatewayName)
{
    SC_TRACE_IN(pszIPAddr, pszGatewayName, 0, 0);

    /* 注意这个地方应该使用 && 符号，只要有一个合法就好 */
    if (DOS_ADDR_INVALID(pszGatewayName) && DOS_ADDR_INVALID(pszIPAddr))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return U32_BUTT;
    }

    /* 去数据库查询 */

    SC_TRACE_OUT();
    return U32_BUTT;
}

U32 sc_get_custom_id(S8 *pszNumber, U32 ulNumType)
{
    SC_TRACE_IN(pszNumber, ulNumType, 0, 0);

    if (DOS_ADDR_INVALID(pszNumber) || ulNumType >= SC_NUM_TYPE_BUTT)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return U32_BUTT;
    }

    SC_TRACE_OUT();
    return U32_BUTT;
}

