#coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: handle the agent
'''
from xml.dom.minidom import Document
import file_info
import db_exec
import agent
import sip


def generate_group(doc, ulGroupID, ulCustomerID):
    '''
    @todo: 生成座席组
    '''
    
    if doc is None:
        file_info.print_file_info('doc is %p' % doc)
        return -1
    
    if str(ulGroupID).strip() == '':
        file_info.print_file_info('ulGroupID does not exist.')
        return -1
    
    if str(ulCustomerID).strip() == '':
        file_info.print_file_info('ulCustomerID does not exist.')
        return -1
    
    seqSQLCmd = 'SELECT id FROM tbl_agent WHERE tbl_agent.group1_id = %d OR tbl_agent.group2_id = %d' % (ulGroupID, ulGroupID)
    listAgent = db_exec.exec_SQL(seqSQLCmd)
    if -1 == listAgent:
        file_info.print_file_info('listAgent is %d' % listAgent)
        return -1
    
    # 将xml转化为DOM
    domGroupNode = doc.createElement('group')
    domGroupNode.setAttribute('name', str(ulCustomerID) + '-' + str(ulGroupID))
    domUsersNode = doc.createElement('users')
    
    for loop in range(len(listAgent)):
        # 根据agentid获取sipid
        ulSipID = agent.get_sipid_by_agentid(int(listAgent[loop][0]))
        if -1 == ulSipID:
            file_info.print_file_info('ulSipID is %d' % ulSipID)
            return -1
        
        # 根据sipid获取userid
        seqUserID = sip.get_userid_by_sipid(ulSipID)
        if -1 == seqUserID:
            file_info.print_file_info('seqUserID is %d' % seqUserID)
            return -1
        
        domUserNode = doc.createElement('user')
        domUserNode.setAttribute('id', seqUserID)
        domUserNode.setAttribute('type', 'pointer')
        domUsersNode.appendChild(domUserNode)
        
        lRet = sip.generate_sip(seqUserID)
        if -1 == lRet:
            file_info.print_file_info('lRet is %d' % lRet)
            return -1
        domGroupNode.appendChild(domUsersNode)
        return domGroupNode
        