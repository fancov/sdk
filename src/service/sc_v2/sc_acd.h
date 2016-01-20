/**
 * @file : sc_acd.h
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 定义坐席管理相关结构
 *
 * @date: 2016年1月14日
 * @arthur: Larry
 */

#ifndef __SC_ACD_H__
#define __SC_ACD_H__

U32 sc_agent_init(U32 ulIndex);
U32 sc_agent_group_init(U32 ulIndex);
U32 sc_agent_status_update(U32 ulAction, U32 ulAgentID, U32 ulOperatingType);
U32 sc_agent_http_update_proc(U32 ulAction, U32 ulAgentID, S8 *pszUserID);
U32 sc_number_lmt_update_proc(U32 ulAction, U32 ulNumlmtID);

#endif /* end of __SC_ACD_H__ */


