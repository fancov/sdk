#coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: 
'''

from xml.dom.minidom import Document
import os
import db_exec
import conf_path
import file_info
import dom_to_xml
import sip
import log


def generate_all_customer():
    '''
    @todo: 生成客户
    '''
    # 创建'/var/log/dipcc/'
    lRet = log.create_fs_log_dir()
    if -1 == lRet:
        file_info.print_file_info('mkdir directory \'/var/log/dipcc\' failed!')
        return -1
    # 清空所有账户
    clean_all_customer()
    # 获取所有客户
    listCus = get_all_customer()
    if -1 == listCus:
        file_info.print_file_info('listCus is %d' % listCus)
        return -1
    
    for loop in range(len(listCus)):
        lRet = generate_customer(int(listCus[loop][0]))
        if -1 == lRet:
            file_info.print_file_info('Customer %d generated failed!' % int(listCus[loop][0]))
    return 1


def get_all_customer():
    '''
    @todo: 获取所有的客户
    '''

    seqSQLCmd = 'SELECT DISTINCT customer_id FROM tbl_sip;'
    listCus = db_exec.exec_SQL(seqSQLCmd)
    if -1 == listCus:
        file_info.print_file_info(seqSQLCmd)
        return -1

    return listCus

def get_sip_by_customer(ulCustomerID):
    '''
    @todo: 根据customer获取所有sip账户
    '''

    if str(ulCustomerID).strip() == '':
        file_info.print_file_info('ulCustomerID does not exist...')
        return -1

    seqSQLCmd = 'SELECT DISTINCT id FROM tbl_sip WHERE customer_id = %d;' % ulCustomerID
    listSip = db_exec.exec_SQL(seqSQLCmd)
    if -1 == listSip:
        file_info.print_file_info('listSip is %d' % listSip)
        return -1

    return listSip

def generate_customer(ulCustomerID):
    '''
    @todo: 生成客户相关文件
    '''

    if str(ulCustomerID).strip() == '':
        file_info.print_file_info('ulCustomerID does not exist...')
        return -1

    # 根据客户id获取sip列表
    listSip = get_sip_by_customer(ulCustomerID)
    if -1 == listSip:
        file_info.print_file_info('listSip is %d' % listSip)
        return -1

    # 获取配置文件路径
    seqFsPath = conf_path.get_config_path()

    # 构造以'/'为结尾的路径字符串
    if seqFsPath[-1] != '/':
        seqFsPath = seqFsPath + '/'
        
    seqFsPath = seqFsPath + 'directory/'
    seqMgntFile = (seqFsPath + 'default.xml' if ulCustomerID == 0 else seqFsPath + str(ulCustomerID) + '.xml')
    
    seqMgntDir = (seqFsPath + 'default' + '/' if ulCustomerID == 0 else seqFsPath + str(ulCustomerID) + '/')
    if os.path.exists(seqMgntDir) is False:
        os.makedirs(seqMgntDir)
        
    doc = Document()
    
    (domIncludeNode, domGroupsNode) = create_customer_head(doc)
    
    domGroupNode = doc.createElement('group')
    domGroupNode.setAttribute('name', 'default')
    
    domUsersNode = doc.createElement('users')
    domGroupNode.appendChild(domUsersNode)
    domPreNode = doc.createElement('X-PRE-PROCESS')
    domPreNode.setAttribute('cmd', 'include')
    domPreNode.setAttribute('data', 'default.xml' if ulCustomerID == 0 else str(ulCustomerID) + '/*.xml')
    domUsersNode.appendChild(domPreNode)
    
    _domGroupNode = doc.createElement('group')
    _domGroupNode.setAttribute('name', 'all')
    _domUsersNode = doc.createElement('users')
    _domGroupNode.appendChild(_domUsersNode)
        
    for loop in range(len(listSip)):
        # 根据sipid获取userid
        seqUserID = sip.get_userid_by_sipid(int(listSip[loop][0]))
        if seqUserID == -1:
            file_info.print_file_info('seqUserID is %d' % seqUserID)
            return -1
        lRet = sip.generate_sip(seqUserID)
        if -1 == lRet:
            file_info.print_file_info('lRet is %d' % lRet)
            return -1
        _domUserNode = doc.createElement('user')
        _domUserNode.setAttribute('id', seqUserID)
        _domUserNode.setAttribute('type', 'pointer')
        _domUsersNode.appendChild(_domUserNode)
    
    domGroupsNode.appendChild(domGroupNode)
    domGroupsNode.appendChild(_domGroupNode)
    doc.appendChild(domIncludeNode)
    
    lRet = dom_to_xml.dom_to_xml(seqMgntFile, doc)
    if -1 == lRet:
        file_info.print_file_info('lRet is %d' % lRet)
        return -1
    lRet = dom_to_xml.del_xml_head(seqMgntFile)
    if -1 == lRet:
        file_info.print_file_info('lRet is %d' % lRet)
        return -1
    
    return 1
              
def create_customer_head(doc):   
    '''
    @todo: 生成客户文件的头部
    '''   
    
    domIncludeNode = doc.createElement('include')
    domDomainNode  = doc.createElement('domain')
    domDomainNode.setAttribute('name', '$${domain}')
    domIncludeNode.appendChild(domDomainNode)
    
    domParamsNode = doc.createElement('params')
    domParamNode  = doc.createElement('param')
    domParamNode.setAttribute('name', 'dial-string')
    domParamNode.setAttribute('value', '{^^:sip_invite_domain=${dialed_domain}:presence_id=${dialed_user}@${dialed_domain}}${sofia_contact(*/${dialed_user}@${dialed_domain})}')
    domParamsNode.appendChild(domParamNode)
    
    listParam = ['record_stereo', 'default_gateway', 'default_areacode', 'transfer_fallback_extension']
    listValue = ['true', '$${default_provider}', '$${default_areacode}', 'operator']
    domVariablesNode = doc.createElement('variables')
    for loop in range(len(listParam)):
        domVariableNode = doc.createElement('variable')
        domVariableNode.setAttribute('name', listParam[loop])
        domVariableNode.setAttribute('value', listValue[loop])
        domVariablesNode.appendChild(domVariableNode)
        
    domDomainNode.appendChild(domParamsNode)
    domDomainNode.appendChild(domVariablesNode)
    
    domGroupsNode = doc.createElement('groups')
    domDomainNode.appendChild(domGroupsNode)
    
    return (domIncludeNode, domGroupsNode)

def clean_all_customer():
    '''
    @todo: 清空所有的账户
    '''

    # 获取配置文件路径
    seqFsPath = conf_path.get_config_path()
    
    # 构造以'/'结尾的路径字符串
    if seqFsPath[-1] != '/':
        seqFsPath = seqFsPath + '/'
        
    # 构造客户目录
    seqCusDir = seqFsPath + 'directory/'
    if os.path.exists(seqCusDir) is False:
        file_info.print_file_info('All customers were cleand.')
        return 1
    
    # 遍历目录并删掉所有文件
    listFile = os.listdir(seqCusDir)
    for item in listFile:
        # 如果是目录，则执行'rm -rf dirname'
        if os.path.isdir(item):
            os.system('rm -rf %s' % os.path.abspath(item))
        else:
            os.system('rm %s' % os.path.abspath(item))
    
    file_info.print_file_info('Customers clean finished.')
    return 1
	