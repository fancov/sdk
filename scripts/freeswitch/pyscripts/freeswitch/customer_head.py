# -*- encoding=utf-8 -*-
'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: generate the head of customer
'''
import file_info

def generate_customer_head(filename, doc):
    if filename is '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'file name is',filename
        return
    if doc is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'doc is', doc
        return
    param_node = doc.createElement('param')
    param_node.setAttribute('name', 'dial-string')
    param_node.setAttribute('value', '{^^:sip_invite_domain=${dialed_domain}:presence_id=${dialed_user}@${dialed_domain}}${sofia_contact(*/${dialed_user}@${dialed_domain})}')
    
    params_node = doc.createElement('params')
    params_node.appendChild(param_node)
    
    dict = {'record_stereo':'true', 'default_gateway':'$${default_provider}'
            , 'default_areacode':'$${default_areacode}', 'transfer_fallback_extension':'operator'}
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'dict is', dict
    variables_node = doc.createElement('variables')
    vari_node = []
    loop = 0
    for key in dict:
        variable = doc.createElement('variable')
        vari_node.append(variable)
        vari_node[loop] = doc.createElement('variable') 
        vari_node[loop].setAttribute('name', key)
        vari_node[loop].setAttribute('value', dict[key])
        variables_node.appendChild(vari_node[loop])
        loop = loop + 1
        
    pre_node = doc.createElement('X-PRE-PROCESS')
    pre_node.setAttribute('cmd', 'include')
    f_name = filename[:-4].split('/')[-1]
    print f_name

    pre_node.setAttribute('data', f_name + '/*.xml')
    
    users_node = doc.createElement('users')
    users_node.appendChild(pre_node)
    
    group_node = doc.createElement('group')
    group_node.setAttribute('name', f_name)
    group_node.appendChild(users_node)
    
    groups_node = doc.createElement('groups')
    groups_node.appendChild(group_node)
    
    domain_node = doc.createElement('domain')
    domain_node.setAttribute('name', '$${domain}')
    domain_node.appendChild(params_node)
    domain_node.appendChild(variables_node)
    domain_node.appendChild(groups_node)
    
    include_node = doc.createElement('include')
    include_node.appendChild(domain_node)
    
    return (include_node, groups_node)

