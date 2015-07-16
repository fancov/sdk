# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: 
'''

from xml.dom.minidom import Document
from xml.dom import minidom
import os
import file_info
import dom_to_xml
import conf_path
import db_conn

def generate_sip(userid):
    '''
    @todo: 生成sip账户
    '''
    if str(userid).strip() == '':
        file_info.print_file_info('Generate SIP %d FAIL.' % int(userid))
        return -1
    # 获取sip相关信息
    listSipInfo = db_conn.get_sipinfo_by_userid(userid)
    if -1 == listSipInfo:
        file_info.print_file_info('Generate SIP %d FAIL.' % int(userid))
        return -1
    ulSipID = int(listSipInfo[0][0])
    seqDispName = listSipInfo[0][3]
    seqAuthName = listSipInfo[0][4]
    seqAuthPassword = listSipInfo[0][5]

    # 获取配置文件路径
    seqFsPath = conf_path.get_config_path()
    # 构造以'/'结尾的路径字符串
    if seqFsPath[-1] != '/':
        seqFsPath = seqFsPath + '/'
    #构造customer目录
    seqFsPath = seqFsPath + 'directory/'
    if os.path.exists(seqFsPath) is False:
        os.makedirs(seqFsPath)

    #构造sip目录
    seqSipDir = seqFsPath + 'default/'
    if os.path.exists(seqSipDir) is False:
        os.makedirs(seqSipDir)

    #构造sip配置文件路径
    seqSipFile = seqSipDir + str(ulSipID) + '.xml'

    #构造XML的DOM对象
    doc = Document()
    
    domParamsNode = doc.createElement('params')
    domVariablesNode = doc.createElement('variables')
    
    dictParam = {'password':'$${default_password}' if listSipInfo == [] else seqAuthPassword, 'vm-password':ulSipID}
    dictVariable = {'toll_allow':'domestic,international,local',
                'accountcode':seqAuthName,
                'user_context':'default',
                'effective_caller_id_name':userid if listSipInfo == [] else seqDispName,
                'effective_caller_id_number':userid,
                'outbound_caller_id_name':'$${outbound_caller_name}',
                'outbound_caller_id_number':'$${outbound_caller_id}',
                'callgroup':'all'
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
    domUserNode.setAttribute('id', userid)
    domUserNode.appendChild(domParamsNode)
    domUserNode.appendChild(domVariablesNode)
        
    domIncludeNode = doc.createElement('include')
    domIncludeNode.appendChild(domUserNode)
    doc.appendChild(domIncludeNode)
    
    # 将DOM对象转化为XML
    lRet = dom_to_xml.dom_to_xml(seqSipFile, doc)
    if -1 == lRet:
        file_info.print_file_info('lRet is %d' % lRet)
        return -1
    
    # 去掉XML文件的头部声明
    lRet = dom_to_xml.del_xml_head(seqSipFile)
    if -1 == lRet:
        file_info.print_file_info('lRet is %d' % lRet)
        return -1

    file_info.print_file_info('Generate SIP %d SUCC.' % int(userid))
    return 1

def add_sip(ulSipID):
    '''
    @增加一个sip账户
    '''
    if str(ulSipID).strip() == '':
        file_info.print_file_info('Add new SIP FAIL.')
        return -1
    # 根据sipid获取userid
    seqUserID = db_conn.get_userid_by_sipid(ulSipID)
    if -1 == seqUserID:
        file_info.print_file_info('Add new SIP FAIL, seqUserID is %d.' % seqUserID)
        return -1
    # 获取配置文件路径
    seqFsPath = conf_path.get_config_path()
    if seqFsPath[-1] != '/':
        seqFsPath = seqFsPath + '/'
    
    # 获取customer路径
    seqFsPath = seqFsPath + 'directory/'
    
    #获取管理xml
    seqMgntFile = seqFsPath + 'default.xml'
    
    # 生成sip配置文件
    lRet = generate_sip(seqUserID)
    if lRet == -1:
        file_info.print_file_info('Add new SIP FAIL, lRet is %d.' % lRet)
        return -1
    
    xmlDoc = minidom.parse(seqMgntFile)
    groupList = xmlDoc.getElementsByTagName('group')
    
    for i in range(len(groupList)):
        if groupList[i].getAttribute('name') == 'all':
            userNode = xmlDoc.createElement('user')
            userNode.setAttribute('id', seqUserID)
            userNode.setAttribute('type', 'pointer')
            
            usersNode = groupList[i].getElementsByTagName('users')
            usersNode[0].appendChild(userNode)
            
            lRet = dom_to_xml.dom_to_xml(seqMgntFile, xmlDoc)
            if lRet == -1:
                file_info.print_file_info('Add new SIP FAIL, lRet is %d.' % lRet)
                return -1
            
            # 删除XML头部声明
            lRet = dom_to_xml.del_xml_head(seqMgntFile)
            if -1 == lRet:
                file_info.print_file_info('Add new SIP FAIL, lRet is %d.' % lRet)
                return -1

        lRet = dom_to_xml.del_blank_line(seqMgntFile)
        if -1 == lRet:
            file_info.print_file_info('Delete Blank Line FAIL,lRet is %d.' % lRet)
            return -1

    file_info.print_file_info('Add new SIP(%s) SUCC.' % str(seqUserID))
    return 0


def del_sip(ulSipID, seqUserID, ulCustomerID):
    '''
    @todo: 删除sip账户
    '''
    if str(ulSipID).strip() == '':
        file_info.print_file_info('Delete SIP FAIL.')
        return -1
    
    if seqUserID.strip() == '':
        file_info.print_file_info('Delete SIP FAIL.')
        return -1
    
    if str(ulCustomerID).strip() == '':
        file_info.print_file_info('Delete SIP FAIL.')
        return -1
    
    # 获取配置文件路径
    seqFsPath = conf_path.get_config_path()
    
    # 构造以'/'结尾的路径字符串
    if seqFsPath[-1] != '/':
        seqFsPath = seqFsPath + '/'
    
    # 构造customer目录
    seqMgntDir = seqFsPath + 'directory/'
    
    # 获取sip文件并删除
    seqSipPath = seqMgntDir + 'default/' + str(ulSipID) + '.xml'
    if os.path.exists(seqSipPath):
        os.remove(seqSipPath)
    
    # 构造管理文件
    seqMgntFile = seqMgntDir + 'default.xml'
    
    if os.path.exists(seqMgntFile):
        xmlDoc = minidom.parse(seqMgntFile)
        groupList = xmlDoc.getElementsByTagName('group')
        
        for i in range(len(groupList)):
            if groupList[i].getAttribute('name') == 'all':
                break
            
        if i < len(groupList):
            usersNode = groupList[i].getElementsByTagName('users')
            userList = usersNode[0].getElementsByTagName('user')
            for j in range(len(userList)):
                if userList[j].getAttribute('id') == seqUserID:
                    usersNode[0].removeChild(userList[j])
                    
        # 写配置文件
        # 将DOM转换为XML
        lRet = dom_to_xml.dom_to_xml(seqMgntFile, xmlDoc)
        if lRet == -1:
            file_info.print_file_info('Delete SIP FAIL, lRet is %d.' % xmlDoc)
            return -1

        # 删除XML头部声明
        lRet = dom_to_xml.del_xml_head(seqMgntFile)
        if -1 == lRet:
            file_info.print_file_info('Delete SIP FAIL, lRet is %d.' % lRet)
            return -1

        lRet = dom_to_xml.del_blank_line(seqMgntFile)
        if -1 == lRet:
            file_info.print_file_info('Delete SIP FAIL, lRet is %d.' % lRet)
            return -1

    file_info.print_file_info('Delete SIP(%s) SUCC.' % seqUserID)
    return 1

