# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: generate the head of customer
'''
import file_info

def generate_customer_head(seqFileName, doc):
    '''
    @param filename: 文件名
    @param doc: 文件对象 
    @todo: 生成客户配置文件的头部
    '''
    if str(seqFileName).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'file name is',seqFileName
        return
    if doc is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'doc is', doc
        return
    domParamNode = doc.createElement('param')
    domParamNode.setAttribute('name', 'dial-string')
    domParamNode.setAttribute('value', '{^^:sip_invite_domain=${dialed_domain}:presence_id=${dialed_user}@${dialed_domain}}${sofia_contact(*/${dialed_user}@${dialed_domain})}')
    
    domParamsNode = doc.createElement('params')
    domParamsNode.appendChild(domParamNode)
    
    dict = {'record_stereo':'true', 'default_gateway':'$${default_provider}'
            , 'default_areacode':'$${default_areacode}', 'transfer_fallback_extension':'operator'}
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'dict is', dict
    domVariablesNode = doc.createElement('variables')
    arrVariNode = []
    loop = 0
    for key in dict:
        variable = doc.createElement('variable')
        arrVariNode.append(variable)
        arrVariNode[loop] = doc.createElement('variable') 
        arrVariNode[loop].setAttribute('name', key)
        arrVariNode[loop].setAttribute('value', dict[key])
        domVariablesNode.appendChild(arrVariNode[loop])
        loop = loop + 1
        
    domPreNode = doc.createElement('X-PRE-PROCESS')
    domPreNode.setAttribute('cmd', 'include')
    seqFName = seqFileName[:-4].split('/')[-1]

    domPreNode.setAttribute('data', seqFName + '/*.xml')
    
    domUsersNode = doc.createElement('users')
    domUsersNode.appendChild(domPreNode)
    
    domGroupNode = doc.createElement('group')
    domGroupNode.setAttribute('name', seqFName)
    domGroupNode.appendChild(domUsersNode)
    
    domGroupsNode = doc.createElement('groups')
    domGroupsNode.appendChild(domGroupNode)
    
    domDomainNode = doc.createElement('domain')
    domDomainNode.setAttribute('name', '$${domain}')
    domDomainNode.appendChild(domParamsNode)
    domDomainNode.appendChild(domVariablesNode)
    domDomainNode.appendChild(domGroupsNode)
    
    domIncludeNode = doc.createElement('include')
    domIncludeNode.appendChild(domDomainNode)
    
    return (domIncludeNode, domGroupsNode)

