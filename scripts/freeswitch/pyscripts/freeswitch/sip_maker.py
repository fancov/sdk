# -*- encoding:utf-8 -*-

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

def make_sip(sip_id, customer_id = 'default', path = '../cfg/'):
    if sip_id == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'sip_id is ', sip_id
        return
    flag_sip = 0
    flag_customer = 0
    if path[-1] <> '/':
        path = path + '/'
    
    if type(customer_id) is types.IntType:
        path = path + str(customer_id) + '/'
    elif type(customer_id) is types.StringType:
        path = path + customer_id + '/'
        flag_customer = 1
        
    if not os.path.exists(path):
        os.makedirs(path)
        
    if type(sip_id) is types.IntType:
        path = path + str(sip_id) + '.xml'
    elif type(sip_id) is types.StringType:
        path = path + sip_id + '.xml'
        flag_sip = 1
    
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), path
        
    doc = Document()
    
    params_node = doc.createElement('params')
    variables_node = doc.createElement('variables')
    
    param_dict = {'password':'$${default_password}', 'vm-password':sip_id if flag_sip == 1 else str(sip_id)}
    variable_dict = {'toll_allow':'domestic,international,local',
            'accountcode':sip_id if flag_sip == 1 else str(sip_id),
            'user_context':customer_id if flag_customer == 1 else str(customer_id),
            'effective_caller_id_name':sip_id if flag_sip == 1 else str(sip_id),
            'effective_caller_id_number':sip_id if flag_sip == 1 else str(sip_id),
            'outbound_caller_id_name':'$${outbound_caller_name}',
            'outbound_caller_id_number':'$${outbound_caller_id}',
            'callgroup':'techsupport'
    }
    
    prms_node = []
    loop = 0
    for key in param_dict:
        param_node = doc.createElement('param')
        param_node.setAttribute('name', key)
        param_node.setAttribute('value', param_dict[key])
        prms_node.append(param_node)
        params_node.appendChild(prms_node[loop])
        loop = loop + 1
        
    vari_node = []
    loop = 0
    for key in variable_dict:
        variable_node = doc.createElement('variable')
        variable_node.setAttribute('name', key)
        variable_node.setAttribute('value', variable_dict[key])
        vari_node.append(variable_node)
        variables_node.appendChild(vari_node[loop])
        loop = loop + 1
        
    user_node = doc.createElement('user')
    user_node.setAttribute('id', str(sip_id))
    user_node.appendChild(params_node)
    user_node.appendChild(variables_node)
    
    include_node = doc.createElement('include')
    include_node.appendChild(user_node)
    doc.appendChild(include_node)
    dom_to_xml.dom_to_pretty_xml(path, doc)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),path
    
        