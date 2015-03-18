#coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: handle the customer
'''

import os
from xml.dom.minidom import Document
import file_info
import db_exec
import db_config
import group
import dom_to_xml

def generate_customer():
    '''
    @todo: 生成一个客户
    '''
    
    # 获取customer列表
    listCustomer = get_all_customer()
    if -1 == listCustomer:
        file_info.print_file_info('listCustomer is %d' % listCustomer)
        return -1
    
    # 获取配置文件路径
    _dict = db_config.get_db_param()
    if -1 == _dict:
        file_info.print_file_info('_dict is %d' % _dict)
        return -1
    seqFsPath = _dict['fs_config_path']
    
    # 构造以'/'为结尾的路径字符串
    if seqFsPath[-1] != '/':
        seqFsPath = seqFsPath + '/'
    
    # 设置customer目录
    seqCusDir = seqFsPath + 'directory/'
    if os.path.exists(seqCusDir) is False:
        os.makedirs(seqCusDir)
    
    # 生成客户配置
    for loop in range(len(listCustomer)):
        seqSipPath = seqCusDir + str(listCustomer[loop][0]) + '/'
        if os.path.exists(seqSipPath) is False:
            os.makedirs(seqSipPath)
        
        lRet = generate_customer_file(int(listCustomer[loop][0]))
        if -1 == lRet:
            file_info.print_file_info('lRet is %d' % lRet)
            return -1
        
    return 1
    

def get_all_customer():
    '''
    @todo: 获取所有的customer
    '''
    
    # 获取所有的customer
    seqSQLCmd = 'SELECT DISTINCT customer_id FROM tbl_sip'
    listResult = db_exec.exec_SQL(seqSQLCmd)
    
    if -1 == listResult:
        file_info.print_file_info('listResult is %d' % listResult)
        return -1
    
    return listResult


def generate_customer_head(seqFileName, doc):
    '''
    @todo: 生成customer配置文件头部
    '''
    
    if seqFileName.strip() == '':
        file_info.print_file_info('seqFileName does not exist.')
        return -1
    
    # 将XML转转为DOM对象
    domParamNode = doc.createElement('param')
    domParamNode.setAttribute('name', 'dial-string')
    domParamNode.setAttribute('value', '{^^:sip_invite_domain=${dialed_domain}:presence_id=${dialed_user}@${dialed_domain}}${sofia_contact(*/${dialed_user}@${dialed_domain})}')
        
    domParamsNode = doc.createElement('params')
    domParamsNode.appendChild(domParamNode)
        
    _dict = {'record_stereo':'true', 'default_gateway':'$${default_provider}'
                , 'default_areacode':'$${default_areacode}', 'transfer_fallback_extension':'operator'}
    file_info.print_file_info(_dict)
    domVariablesNode = doc.createElement('variables')
    arrVariNode = []
    loop = 0
    for key in _dict:
        variable = doc.createElement('variable')
        arrVariNode.append(variable)
        arrVariNode[loop] = doc.createElement('variable') 
        arrVariNode[loop].setAttribute('name', key)
        arrVariNode[loop].setAttribute('value', _dict[key])
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

def generate_customer_file(ulCustomerID):
    '''
    @todo: 生成客户配置文件
    '''
    
    if str(ulCustomerID).strip() == '':
        file_info.print_file_info()
        return -1
    
    # 找出所有的Group
    seqSQLCmd = 'SELECT DISTINCT id FROM tbl_group WHERE tbl_group.customer_id = %d' % int(ulCustomerID)
    listGroup = db_exec.exec_SQL(seqSQLCmd)
    if -1 == listGroup:
        file_info.print_file_info('listGroup is %d' % listGroup)
        return -1
    
    # 获取数据库配置信息
    _dict = db_config.get_db_param()
    if -1 == _dict:
        file_info.print_file_info('_dict is %d' % _dict)
        return -1
    # 获取FreeSwitch配置文件路径
    seqFsPath = _dict['fs_config_path']
    # 构造以'/'结尾的字符串
    if seqFsPath[-1] != '/':
        seqFsPath = seqFsPath + '/'
    if os.path.exists(seqFsPath) is False:
        os.makedirs(seqFsPath)
    
    # 构造客户目录
    seqCusDir = seqFsPath + 'directory/' + str(ulCustomerID) + '/'
    if os.path.exists(seqCusDir) is False:
        os.makedirs(seqCusDir)
    
    # 构造管理配置文件
    seqCusFile = seqFsPath + 'directory/' + str(ulCustomerID) + '.xml'
    
    doc = Document()
    
    # xml生成头部
    lRet = generate_customer_head(seqCusFile, doc)
    if -1 == lRet:
        file_info.print_file_info('lRet is %d' % lRet)
        return -1
    
    # 取得根节点和Groups节点
    (domIncludeNode, domGroupsNode) = lRet
    
    # 生成sip群
    for loop in range(len(listGroup)):
        domGroupNode = group.generate_group(doc, int(listGroup[loop][0]), ulCustomerID)
        if -1 == domGroupNode:
            file_info.print_file_info('domGroupNode is %d' % domGroupNode)
            return -1
        
        domGroupsNode.appendChild(domGroupNode)
        doc.appendChild(domIncludeNode)
        
        # 将DOM转换为XML
        lRet = dom_to_xml.dom_to_xml(seqCusFile, doc)
        if lRet == -1:
            file_info.print_file_info('lRet is %d' % lRet)
            return -1
        
        # 删除XML头部声明
        lRet = dom_to_xml.del_xml_head(seqCusFile)
        if -1 == lRet:
            file_info.print_file_info('lRet is %d' % lRet)
            return -1
        
        return 1
