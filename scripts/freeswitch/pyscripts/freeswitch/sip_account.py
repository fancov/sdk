# -*- encoding=utf-8 -*-

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: generate the sip account xml files
'''

from xml.dom.minidom import Document
import dom_to_xml
import os
import types
import file_info
import db_config

def generate_sip_account(sip_id, customer_id):
    '''
    @todo: generate sip account configuration
    '''
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip id is', sip_id
        return
    if str(customer_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer id is', customer_id    
        return
    
    cfg_path = db_config.get_db_param()['cfg_path']
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
    if os.path.exists(cfg_path) is not True:
        os.makedirs(cfg_path) 
    sip_path = cfg_path + 'directory/' + str(customer_id) + '/' + str(sip_id) + '.xml'
    
    doc = Document()
    
    params_node = doc.createElement('params')
    variables_node = doc.createElement('variables')
    
    param_dict = {'password':'$${default_password}', 'vm-password':str(sip_id)}
    variable_dict = {'toll_allow':'domestic,international,local',
            'accountcode':str(sip_id),
            'user_context':str(customer_id),
            'effective_caller_id_name':str(sip_id),
            'effective_caller_id_number':str(sip_id),
            'outbound_caller_id_name':'$${outbound_caller_name}',
            'outbound_caller_id_number':'$${outbound_caller_id}',
            'callgroup':'techsupport'
    }
    
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name,'variable_dict is', variable_dict
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
    
    dom_to_xml.dom_to_pretty_xml(sip_path, doc)
    dom_to_xml.del_xml_head(sip_path)