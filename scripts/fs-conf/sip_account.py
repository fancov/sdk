# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: generate the sip account xml files
'''

from xml.dom.minidom import Document
import dom_to_xml
import os
import file_info
import db_config

def generate_sip_account(ulSIPID, ulCustomerID):
    '''
    @param sip_id: sip账户id
    @param customer_id: sip账户所属客户id
    @todo: generate sip account configuration
    '''
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    if str(ulCustomerID).strip() == '':
        file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
        return -1
    
    seqCfgPath = db_config.get_db_param()['fs_config_path']
    if seqCfgPath is None or seqCfgPath == '' or seqCfgPath == -1:
        file_info.get_cur_runtime_info('The param is invalid.')
        return -1
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    if os.path.exists(seqCfgPath) is False:
        os.makedirs(seqCfgPath) 
    seqSIPPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '/' + str(ulSIPID) + '.xml'
    
    doc = Document()
    
    domParamsNode = doc.createElement('params')
    domVariablesNode = doc.createElement('variables')
    
    dictParam = {'password':'$${default_password}', 'vm-password':str(ulSIPID)}
    dictVariable = {'toll_allow':'domestic,international,local',
            'accountcode':str(ulSIPID),
            'user_context':str(ulCustomerID),
            'effective_caller_id_name':str(ulSIPID),
            'effective_caller_id_number':str(ulSIPID),
            'outbound_caller_id_name':'$${outbound_caller_name}',
            'outbound_caller_id_number':'$${outbound_caller_id}',
            'callgroup':'techsupport'
    }
    
    file_info.get_cur_runtime_info(dictVariable)
    listParamsNode = []
    loop = 0
    for key in dictParam:
        domParamNode = doc.createElement('param')
        domParamNode.setAttribute('name', key)
        domParamNode.setAttribute('value', dictParam[key])
        listParamsNode.append(domParamNode)
        domParamsNode.appendChild(listParamsNode[loop])
        loop = loop + 1
        
    listVariNode = []
    loop = 0
    for key in dictVariable:
        domVariableNode = doc.createElement('variable')
        domVariableNode.setAttribute('name', key)
        domVariableNode.setAttribute('value', dictVariable[key])
        listVariNode.append(domVariableNode)
        domVariablesNode.appendChild(listVariNode[loop])
        loop = loop + 1
        
    domUserNode = doc.createElement('user')
    domUserNode.setAttribute('id', str(ulSIPID))
    domUserNode.appendChild(domParamsNode)
    domUserNode.appendChild(domVariablesNode)
    
    domIncludeNode = doc.createElement('include')
    domIncludeNode.appendChild(domUserNode)
    doc.appendChild(domIncludeNode)
    
    lRet = dom_to_xml.dom_to_pretty_xml(seqSIPPath, doc)
    if -1 == lRet:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
    lRet = dom_to_xml.del_xml_head(seqSIPPath)
    if -1 == lRet:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
    
    return 1