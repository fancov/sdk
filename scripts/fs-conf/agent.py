#coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: operation of agent
'''

import file_info
import db_exec

def get_sipid_by_agentid(ulAgentID):
    '''
    @todo: 根据坐席id获取sip索引id
    '''
    if str(ulAgentID).strip() == '':
        file_info.print_file_info('ulAgentID does not exist...')
        return -1
    
    # 根据坐席id获取索引id
    seqSQLCmd = 'SELECT sip_id FROM tbl_agent WHERE id = %d' % int(ulAgentID)
    listSipID = db_exec.exec_SQL(seqSQLCmd)
    if -1 == listSipID:
        file_info.print_file_info('listSipID is %d' % listSipID)
        return -1
    
    ulSipID = int(listSipID[0][0])
    
    return ulSipID

def get_agentid_by_sipid(ulSipID):
    '''
    @todo: 根据sip id获取agent id
    '''
    
    if str(ulSipID).strip() == '':
        file_info.print_file_info('ulSipID does not exist...')
        return -1
    
    seqSQLCmd = 'SELECT id FROM tbl_agent WHERE sip_id = %d' % ulSipID
    listAgent = db_exec.exec_SQL(seqSQLCmd)
    if -1 == listAgent:
        file_info.print_file_info('listAgent is %d' % listAgent)
        return -1
    
    ulAgentID = int(listAgent[0][0])
    
    return ulAgentID

def get_groupid_by_agentid(ulAgentID):
    '''
    @todo: 根据agent id获取group id，可能不止一个
    '''
    if str(ulAgentID).strip() == '':
        file_info.print_file_info('ulAgentID does not exist...')
        return -1
    
    seqSQLCmd = 'SELECT group1_id, group2_id FROM tbl_agent WHERE id = %d' % ulAgentID
    listGroup = db_exec.exec_SQL(seqSQLCmd)
    if -1 == listGroup:
        file_info.print_file_info('listGroup is %d' % listGroup)
        return -1
    
    return listGroup