/**
 * @file : sc_event_lib.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 实现业务控制模块公共函数
 *
 *
 * @date: 2016年1月18日
 * @arthur: Larry
 */


#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_def.h"
#include "sc_debug.h"

U32 sc_call_ctrl_call_agent(U32 ulCurrentAgent, U32 ulAgentCalled)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_call_sip(U32 ulAgent, S8 *pszSipNumber)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_call_out(U32 ulAgent, U32 ulTaskID, S8 *pszNumber)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_transfer(U32 ulAgent, U32 ulAgentCalled, BOOL bIsAttend)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_hold(U32 ulAgent, BOOL isHold)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_unhold(U32 ulAgent)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_hangup(U32 ulAgent)
{
    return DOS_SUCC;
}


U32 sc_call_ctrl_proc(U32 ulAction, U32 ulTaskID, U32 ulAgent, U32 ulCustomerID, U32 ulType, S8 *pszCallee, U32 ulFlag, U32 ulCalleeAgentID)
{
    return DOS_SUCC;
}

U32 sc_demo_task(U32 ulCustomerID, S8 *pszCallee, S8 *pszAgentNum, U32 ulAgentID)
{
    return DOS_SUCC;
}

U32 sc_demo_preview(U32 ulCustomerID, S8 *pszCallee, S8 *pszAgentNum, U32 ulAgentID)
{
    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* End of __cplusplus */

