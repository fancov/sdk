# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 5th,2015
@todo: generate a sip account
'''

import types
import os
import dom_to_xml
import file_info
from xml.dom.minidom import Document

def make_sip(ulSIPID, ulCustomerID = 'default', seqPath = '../cfg/'):
    '''
    @param sip_id: sip账户id
    @param customer_id: 客户id
    @param path: sip账户配置文件路径
    @todo: 生成sip账户配置文件
    '''
    if ulSIPID == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'sip_id is ', ulSIPID
        return

    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'path:', seqPath
        
    doc = Document()
    
    domParamsNode = doc.createElement('params')
    domVariablesNode = doc.createElement('variables')
    
    dictParam = {'password':'$${default_password}', 'vm-password':ulSIPID}
    dictVariable = {'toll_allow':'domestic,international,local',
            'accountcode':ulSIPID,
            'user_context':ulCustomerID,
            'effective_caller_id_name':ulSIPID,
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
    domUserNode.setAttribute('id', str(ulSIPID))
    domUserNode.appendChild(domParamsNode)
    domUserNode.appendChild(domVariablesNode)
    
    domIncludeNode = doc.createElement('include')
    domIncludeNode.appendChild(domUserNode)
    doc.appendChild(domIncludeNode)
    dom_to_xml.dom_to_pretty_xml(seqPath, doc)
    dom_to_xml.del_xml_head(seqPath)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'path:',seqPath
    
        