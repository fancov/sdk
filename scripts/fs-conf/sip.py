#coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: handle the sip account
'''

from xml.dom.minidom import Document
import os
import db_exec
import file_info
import conf_path
import dom_to_xml
import agent
import db_config


def generate_sip(userid):
    '''
    @todo: 根据userid生成sip账户配置
    '''
    
    if str(userid).strip() == '':
        file_info.print_file_info('userid does not exist.')
        return -1
    
    # 获取sip相关信息
    listResult = get_sipinfo_by_userid(userid)
    if -1 == listResult:
        file_info.print_file_info('listResult is %d' % listResult)
        return -1
    
    ulID = int(listResult[0][0])
    ulCustomerID = int(listResult[0][1])
    seqExtension = listResult[0][2]
    seqDispName = listResult[0][3]
    seqAuthName = listResult[0][4]
    seqAuthPassword = listResult[0][5]
    
    # 获取配置文件路径
    seqFsPath = conf_path.get_config_path()
    if -1 == seqFsPath:
        file_info.print_file_info('seqFsPath is %d' % seqFsPath)
        return -1
    
    # 构造以'/'结尾的路径字符串
    if seqFsPath[-1] != '/':
        seqFsPath = seqFsPath + '/'
    
    # 构造customer目录
    seqFsPath = seqFsPath + 'directory/'
    if os.path.exists(seqFsPath) is False:
        os.makedirs(seqFsPath)
        
    # 构造sip目录
    seqSipDir = seqFsPath + str(ulCustomerID) + '/'
    if os.path.exists(seqSipDir) is False:
        os.makedirs(seqSipDir)
        
    # 构造sip配置文件路径
    seqSipXml = seqSipDir + str(userid) + '.xml'
    
    # 构造XML树DOM对象
    doc = Document()
    
    domParamsNode = doc.createElement('params')
    domVariablesNode = doc.createElement('variables')
    
    dictParam = {'password':'$${default_password}' if listResult == [] else seqAuthPassword, 'vm-password':ulID}
    dictVariable = {'toll_allow':'domestic,international,local',
                'accountcode':seqAuthName,
                'user_context':ulCustomerID,
                'effective_caller_id_name':'' if listResult == [] else seqDispName,
                'effective_caller_id_number':userid,
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
    domUserNode.setAttribute('id', userid)
    domUserNode.appendChild(domParamsNode)
    domUserNode.appendChild(domVariablesNode)
        
    domIncludeNode = doc.createElement('include')
    domIncludeNode.appendChild(domUserNode)
    doc.appendChild(domIncludeNode)
    
    # 将DOM对象转化为XML
    lRet = dom_to_xml.dom_to_xml(seqSipXml, doc)
    if -1 == lRet:
        file_info.print_file_info('lRet is %d' % lRet)
        return -1
    
    # 去掉XML文件的头部声明
    lRet = dom_to_xml.del_xml_head(seqSipXml)
    if -1 == lRet:
        file_info.print_file_info('lRet is %d' % lRet)
        return -1
    
    return 1


def get_sipinfo_by_userid(userid):
    '''
    @todo: 根据userid获取sip相关信息
    '''
    
    # 根据userid获取索引，客户id，extension，备注名，认证名，认证密码信息
    seqSQLCmd = 'SELECT id, customer_id, extension, dispname, authname, auth_password FROM tbl_sip WHERE userid = \'%s\' ' %  userid
    listResult = db_exec.exec_SQL(seqSQLCmd)
    
    if -1 == listResult:
        file_info.print_file_info('listResult is %d' % listResult)
        return -1
    
    return listResult

def get_sipid_by_userid(userid):
    '''
    @todo: 根据userid获取sip索引id
    '''
    
    if str(userid).strip() == '':
        file_info.print_file_info('userid does not exist.')
        return -1
    
    # 查找出userid对应的sip账户索引id
    seqSQLCmd = 'SELECT id FROM tbl_sip WHERE userid = \'%s\' ' % userid
    listResult = db_exec.exec_SQL(seqSQLCmd)
    if -1 == listResult:
        file_info.print_file_info('listResult is %d' % listResult)
        return -1
    
    ulSipID = int(listResult[0][0])
    
    return ulSipID


def get_userid_by_sipid(ulSipID):
    '''
    @todo: 根据userid获取sipid
    '''
    if str(ulSipID).strip() == '':
        file_info.print_file_info('ulSipID does not exist...')
        return -1
    
    # 根据sipid获取userid
    seqSQLCmd = 'SELECT userid FROM tbl_sip WHERE id = %d' % int(ulSipID)
    listResult = db_exec.exec_SQL(seqSQLCmd)
    if -1 == listResult:
        file_info.print_file_info('listResult is %d' % listResult)
        return -1
    
    userid = str(listResult[0][0])
    
    return userid

def get_customerid_by_sipid(ulSipID):
    '''
    @todo: 根据sip id获取customer id
    '''
    
    if str(ulSipID).strip() == '':
        file_info.print_file_info('ulSipID does not exist...')
        return -1
    
    seqSQLCmd = 'SELECT customer_id FROM tbl_sip WHERE id = %d' % ulSipID
    listCus = db_exec.exec_SQL(seqSQLCmd)
    if -1 == listCus:
        file_info.print_file_info('listCus is %d' % listCus)
        return -1
    
    ulCustomerID = int(listCus[0][0])
    
    return ulCustomerID
    

def add_sip(ulSipID):
    '''
    @todo: 增加一个sip分机
    '''
    
    if str(ulSipID).strip() == '':
        file_info.print_file_info('ulSipID does not exist...')
        return -1
    
    # 获取配置文件路径
    seqFsPath = db_config.get_db_param()['fs_config_path']
    
    # 根据sip id获取userid
    seqUserID = get_userid_by_sipid(ulSipID)
    if -1 == seqUserID:
        file_info.print_file_info('seqUserID is %d' % seqUserID)
        return -1
    
    # 生成sip配置文件
    lRet = generate_sip(seqUserID)
    if -1 == lRet:
        file_info.print_file_info('lRet is %d' % lRet)
        return -1
    
    # 根据sipid获取agent id
    ulAgentID = agent.get_agentid_by_sipid(ulSipID)
    if -1 == ulAgentID:
        file_info.print_file_info('ulAgentID is %d' % ulAgentID)
        return -1
    
    # 根据agent id获取group id
    listGroup = agent.get_groupid_by_agentid(ulAgentID)
    if -1 == listGroup[0]:
        file_info.print_file_info('listGroup is %d' % listGroup)
        return -1
    # 根据sip id获取customer id
    ulCustomerID = get_customerid_by_sipid(ulSipID)
    if -1 == ulCustomerID:
        file_info.print_file_info('ulCustomerID is %d' % ulCustomerID)
        return -1
    
    for item in listGroup[0]:
        if str(item).strip() == '0':
            continue
        else:
            # 构造以'/'结尾的路径字符串
            if seqFsPath[-1] != '/':
                seqFsPath = seqFsPath + '/'
            # 获取客户目录
            seqCusDir = seqFsPath + 'directory/' + str(ulCustomerID) + '.xml'
            
            # 读取文件
            try:
                listText = open(seqCusDir, 'r').readlines()
            except Exception, err:
                file_info.print_file_info('Catch Exception: %s' % str(err))
                return -1
            else:
                # 构造字符串
                seqAddText = '     <user id=\"%s\" type=\"pointer\"/>' % seqUserID
                if seqAddText in listText:
                    continue
                else:
                    seqGroupText = '   <group name=\"%d-%d\">' % (ulCustomerID, int(item))
                    # 找到改串索引
                    try:
                        ulIndex = listText.index(seqGroupText)
                    except Exception, err:
                        file_info.print_file_info('Catch Exception: %s' % str(err))
                    else:
                        if ulIndex < 0:
                            file_info.print_file_info('ulIndex is %d' % ulIndex)
                            continue
                    
                        listText.insert(ulIndex + 2, seqAddText)
                        open(seqCusDir, 'w').writelines(listText)
    return 1

def del_sip(seqUserID, ulCustomerID):
    '''
    @todo: 删除一个sip分机
    '''
    
    if str(seqUserID).strip() == '0':
        file_info.print_file_info('seqUserID does not exist...')
        return -1
    
    if str(ulCustomerID).strip() == '0':
        file_info.print_file_info('ulCustomerID does not exist...')
        return -1
    
    # 获取配置文件路径
    seqFsPath = db_config.get_db_param()['fs_config_path']
    if seqFsPath[-1] != '/':
        seqFsPath = seqFsPath + '/'
    
    # 构造sip配置文件路径
    seqSipPath = seqFsPath + 'directory/' + str(ulCustomerID) + '/' + seqUserID + '.xml'
    # 构造客户文件路径
    seqCusPath = seqFsPath + 'directory/' + str(ulCustomerID) + '.xml'
    
    if os.path.exists(seqSipPath):
        os.remove(seqSipPath)
    
    if os.path.exists(seqCusPath) is False:
        file_info.print_file_info('file \'%s\' does not exist...' % seqCusPath)
        return 1
    
    # 读取文件
    try:
        listText = open(seqCusPath, 'w').readlines()
    except Exception, err:
        file_info.print_file_info('Catch Exception: %s' % str(err))
        return -1
    else:
        # 构造删除字符串
        seqSipText = '     <user id=\"%s\" type=\"pointer\"/>' % seqUserID
        
        while seqSipText in listText:
            ulIndex = listText.index(seqSipText)
            del listText[ulIndex]
        
        open(seqCusPath, 'w').writelines(listText)
        
        return 1
          