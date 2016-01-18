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

U32 sc_acd_agent_update_status(U32 ulAction, U32 ulAgentID, U32 ulOperatingType);
U32 sc_acd_http_agent_update_proc(U32 ulAction, U32 ulAgentID, S8 *pszUserID);
U32 sc_acd_get_agent_by_emp_num(SC_ACD_AGENT_INFO_ST *pstAgentInfo, U32 ulCustomID, S8 *pszEmpNum);
U32 sc_number_lmt_update_proc(U32 ulAction, U32 ulNumlmtID);

#endif /* end of __SC_ACD_H__ */


