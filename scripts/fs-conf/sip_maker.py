# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 5th,2015
@todo: generate a sip account
'''

import dom_to_xml
import file_info
from xml.dom.minidom import Document
import db_conn

def make_sip(ulSIPID, ulCustomerID = 'default', seqPath = '../cfg/'):
    '''
    @param sip_id: sip�˻�id
    @param customer_id: �ͻ�id
    @param path: sip�˻������ļ�·��
    @todo: ����sip�˻������ļ�
    '''
    if str(ulSIPID) == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1

    file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
    
    listSIPInfo = get_sip_info(ulSIPID)
    if -1 == listSIPInfo:
        file_info.get_cur_runtime_info(listSIPInfo)
        return -1
    
    ulCustomerID = listSIPInfo[0][0]
    seqExtension = listSIPInfo[0][1]
    seqDisplayName = listSIPInfo[0][2]
    seqUserName = listSIPInfo[0][3]
    seqAuthName = listSIPInfo[0][4]
    seqAuthPassword = listSIPInfo[0][5]
        
    doc = Document()
    
    domParamsNode = doc.createElement('params')
    domVariablesNode = doc.createElement('variables')
    
    dictParam = {'password':'$${default_password}' if listSIPInfo == [] else seqAuthPassword, 'vm-password':ulSIPID}
    dictVariable = {'toll_allow':'domestic,international,local',
            'accountcode':seqAuthName,
            'user_context':ulCustomerID,
            'effective_caller_id_name':'' if listSIPInfo == [] else seqDisplayName,
            'effective_caller_id_number':ulSIPID,
            'outbound_caller_id_name':'$${outbound_caller_name}',
            'outbound_caller_id_number':'$${outbound_caller_id}',
            'callgroup':'techsupport'
    }
    
    listParamsNode = []
    loop = 0
    for key in dictParam:
        domParamNode = doc.createElement('param')
        domParamNode.setAttribute('name', str(key))
        domParamNode.setAttribute('value', str(dictParam[key]))
        listParamsNode.append(domParamNode)
        domParamsNode.appendChild(listParamsNode[loop])
        loop = loop + 1
        
    listVariNode = []
    loop = 0
    for key in dictVariable:
        domVariableNode = doc.createElement('variable')
        domVariableNode.setAttribute('name', str(key))
        domVariableNode.setAttribute('value', str(dictVariable[key]))
        listVariNode.append(domVariableNode)
        domVariablesNode.appendChild(listVariNode[loop])
        loop = loop + 1
        
    domUserNode = doc.createElement('user')
    domUserNode.setAttribute('id', seqUserName)
    domUserNode.appendChild(domParamsNode)
    domUserNode.appendChild(domVariablesNode)
    
    domIncludeNode = doc.createElement('include')
    domIncludeNode.appendChild(domUserNode)
    doc.appendChild(domIncludeNode)
    
    lRet = dom_to_xml.dom_to_pretty_xml(seqPath, doc)
    if -1 == lRet:
        return -1
    lRet = dom_to_xml.del_xml_head(seqPath)
    if -1 == lRet:
        return -1
    return 1

def get_sip_info(ulSIPID):
    '''
    @todo: 获取sip账户的信息
    '''
    seqSQLCmd = 'SELECT customer_id, extension, dispname, userid, authname, auth_password FROM tbl_sip WHERE id = %d' % ulSIPID
    file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
    
    try:
        ret = db_conn.connect_db()
    except Exception, err:
        file_info.get_cur_runtime_info('Catch Exception:%s' % str(err))
    else:
        if -1 == ret:
            return -1
        cursor = db_conn.CONN.cursor()
        if cursor is None:
            file_info.get_cur_runtime_info('The database connection does not exist.')
            return -1
        
        cursor.execute(seqSQLCmd)
        listResult = cursor.fetchall()
        if len(listResult) == 0:
            file_info.get_cur_runtime_info('len(listResult) is %d' % len(listResult))
            return -1
        
        file_info.get_cur_runtime_info(listResult)
        db_conn.CONN.close()
        
        print listResult
        
        return listResult