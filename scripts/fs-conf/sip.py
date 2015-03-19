#coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: 
'''

from xml.dom.minidom import Document
import os
import file_info
import dom_to_xml
import conf_path
import db_exec
    
def generate_sip(userid):
    '''
    @todo: 生成sip账户
    '''
    if str(userid).strip() == '':
        file_info.print_file_info('userid does not exist...')
        return -1
    # 获取sip相关信息
    listSipInfo = get_sipinfo_by_userid(userid)
    if -1 == listSipInfo:
        file_info.print_file_info('listSipInfo is %d' % listSipInfo)
        return -1
    ulSipID = int(listSipInfo[0][0])
    ulCustomerID = int(listSipInfo[0][1])
    seqExtension = listSipInfo[0][2]
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
    seqSipDir = (seqFsPath + 'default/' if ulCustomerID == 0 else seqFsPath + str(ulCustomerID) + '/')
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
                'user_context':str(ulCustomerID) if ulCustomerID != 0 else 'default',
                'effective_caller_id_name':userid if listSipInfo == [] else seqDispName,
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
    lRet = dom_to_xml.dom_to_xml(seqSipFile, doc)
    if -1 == lRet:
        file_info.print_file_info('lRet is %d' % lRet)
        return -1
    
    # 去掉XML文件的头部声明
    lRet = dom_to_xml.del_xml_head(seqSipFile)
    if -1 == lRet:
        file_info.print_file_info('lRet is %d' % lRet)
        return -1
    
    return 1

def get_sipinfo_by_userid(userid):
    '''
    @todo: 根据userid获取sip相关信息
    '''

    if str(userid).strip() == '':
        file_info.print_file_info('userid does not exist...')
        return -1

    # 根据userid获取sip相关信息
    seqSQLCmd = 'SELECT id, customer_id, extension, dispname, authname, auth_password FROM tbl_sip WHERE userid = \'%s\';' % str(userid)
    listSipInfo = db_exec.exec_SQL(seqSQLCmd)
    if -1 == listSipInfo:
        file_info.print_file_info('listSipInfo is %d' % listSipInfo)
        return -1

    return listSipInfo

def get_userid_by_sipid(ulSipID):
    '''
    @todo: 根据userid获取
    '''
    
    if str(ulSipID).strip() == '':
        file_info.print_file_info('userid does not exist.')
        return -1
    
    seqSQLCmd = 'SELECT userid FROM tbl_sip WHERE id=%d;' % ulSipID
    lRet = db_exec.exec_SQL(seqSQLCmd)
    if -1 == lRet:
        file_info.print_file_info('lRet is %d' % lRet)
        return -1
    
    seqUserID = str(lRet[0][0])
    
    return seqUserID

def get_customerid_by_sipid(ulSipID):
    '''
    @todo: 根据sipid获取customerid
    '''
    if str(ulSipID).strip() == '':
        file_info.print_file_info('ulSipID does not exist.')
        return -1
    
    seqSQLCmd = 'SELECT customer_id FROM tbl_sip WHERE id = %d;' % ulSipID
    lRet = db_exec.exec_SQL(seqSQLCmd)
    if -1 == lRet:
        file_info.print_file_info('lRet is %d' % lRet)
        return -1
    
    ulCustomerID = int(lRet[0][0])
    return ulCustomerID
    

def add_sip(ulSipID):
    '''
    @增加一个sip账户
    '''
    if str(ulSipID).strip() == '':
        file_info.print_file_info('ulSipID')
        return -1
    # 根据sipid获取userid
    seqUserID = get_userid_by_sipid(ulSipID)
    if -1 == seqUserID:
        file_info.print_file_info('seqUserID is %d' % seqUserID)
        return -1
    # 根据sipid获取customerid
    ulCustomerID = get_customerid_by_sipid(ulSipID)
    if -1 == ulCustomerID:
        file_info.print_file_info('ulCustomerID is %d' % ulCustomerID)
        return -1
    # 获取配置文件路径
    seqFsPath = conf_path.get_config_path()
    if seqFsPath[-1] != '/':
        seqFsPath = seqFsPath + '/'
    
    # 获取customer路径
    seqFsPath = seqFsPath + 'directory/'
    
    #获取管理xml
    seqMgntFile = seqFsPath + ('default.xml' if ulCustomerID == 0 else str(ulCustomerID) + '.xml')
    
    # 生成sip配置文件
    lRet = generate_sip(seqUserID)
    if lRet == -1:
        file_info.print_file_info('lRet is %d' % lRet)
        return -1
    
    # 读取管理xml
    try:
        listText = open(seqMgntFile, 'r').readlines()
    except Exception, err:
        file_info.print_file_info('Catch Exception: %s' % str(err))
        return -1
    else:
        # 构造需要添加的行
        seqAddText = '     <user id=\"%s\" type=\"pointer\"/>\n' % seqUserID
        # 构造需要查找的行
        seqFindText = '   </group>\n'
        # 查找第一个'</group>的位置'
        try:
            ulIndex = listText.index(seqFindText)
        except Exception, err:
            file_info.print_file_info('Catch Exception: %s' % str(err))
            return -1
        else:
            # 添加到相应位置
            if seqAddText in listText:
                file_info.print_file_info('Content exists,needn\'t add')
                return 1
            else:
                listText.insert(ulIndex + 3, seqAddText)
            # 重新写进文件
            open(seqMgntFile, 'w').writelines(listText)
            
            return 1
        
def del_sip(ulSipID, seqUserID, ulCustomerID):
    '''
    @todo: 删除sip账户
    '''
    if str(ulSipID).strip() == '':
        file_info.print_file_info('ulSipID does not exist..')
        return -1
    
    if seqUserID.strip() == '':
        file_info.print_file_info('seqUserID does not exist..')
        return -1
    
    if str(ulCustomerID).strip() == '':
        file_info.print_file_info('ulCustomerID does not exist..')
        return -1
    
    # 获取配置文件路径
    seqFsPath = conf_path.get_config_path()
    
    # 构造以'/'结尾的路径字符串
    if seqFsPath[-1] != '/':
        seqFsPath = seqFsPath + '/'
    
    # 构造customer目录
    seqMgntDir = seqFsPath + 'directory/'
    
    # 获取sip文件并删除
    seqSipPath = seqMgntDir + ('default/' if ulCustomerID == 0 else str(ulCustomerID) + '/') + str(ulSipID) + '.xml'
    if os.path.exists(seqSipPath):
        os.remove(seqSipPath)
    
    # 构造管理文件
    seqMgntFile = seqMgntDir + ('default.xml' if ulCustomerID == 0 else str(ulCustomerID) + '.xml')
    
    # 读取管理文件
    try:
        listText = open(seqMgntFile, 'r').readlines()
    except Exception, err:
        file_info.print_file_info('Catch Exception: %s' % str(err))
        return -1
    else:
        # 构造删除行
        seqDelText = '     <user id=\"%s\" type=\"pointer\"/>\n' % seqUserID
        
        while seqDelText in listText:
            # 找到删除航
            try:
                ulIndex = listText.index(seqDelText)
            except Exception, err:
                file_info.print_file_info('Catch Exception: %s' % str(err))
                return -1
            else:
                del listText[ulIndex]
        
        # 重新写进xml
        open(seqMgntFile, 'w').writelines(listText)
        
        return 1
    