# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 9rd,2015
@todo: generate the directories and management xml files
'''
from xml.dom.minidom import Document
import db_conn
import file_info
import db_config
import os
import dom_to_xml
from __builtin__ import str

def phone_route():
    '''
    @todo: 电话路由
    '''
    global CONN
    lRet = db_conn.connect_db()
    if -1 == lRet:
        file_info.print_file_info('Database Connect FAIL,lRet is %d' % lRet)
        return -1
  
    # 查找所有的路由id
    seqSQLCmd = 'SELECT DISTINCT CONVERT(id, CHAR(10)) AS id FROM tbl_gateway;'
    file_info.print_file_info('seqSQLCmd is %s' % seqSQLCmd)
    results = db_conn.exec_SQL(seqSQLCmd)
    if -1 == results:
        file_info.print_file_info('Phone route FAIL,results == %d' % results)
        return -1
    
    for loop in range(len(results)):
        make_route(int(results[loop][0]))

    file_info.print_file_info('Phone Route SUCC.')
    return 1


def make_route(ulGatewayID):
    '''
    @param ulGatewayID: ulGatewayID 网关ID
    @todo: 生成网关路由
    '''
    doc = Document()
    results = db_conn.get_route_param(ulGatewayID)
    if -1 == results:
        file_info.print_file_info('Generate Route Configuration FAIL, results is %d' % results)
        return -1
    domGatewayNode = doc.createElement('gateway')
    domGatewayNode.setAttribute('name', str(ulGatewayID))
    
    listParamNames  = ['username', 'realm', 'from-user', 'from-domain', 'password', 'extension', 'proxy', 'register-proxy', 'expire-seconds', 'register', 'reg_transport', 'retry_secs', 'caller-id-in-from', 'contact_params','exten_in_contact', 'ping']
    listParamValues = [results[0][1], results[0][3], results[0][4], results[0][5], results[0][2], results[0][6], results[0][7], results[0][9], results[0][8], results[0][10], results[0][11], results[0][12], results[0][13], results[0][14], results[0][15], results[0][16]]
    
    for loop in range(len(listParamNames)): 
        if str(listParamValues[loop]).strip() == '' or str(listParamValues[loop]).strip() == 'None':
            continue
        domParamNode = doc.createElement('param')
        domParamNode.setAttribute('name', listParamNames[loop].strip())
        
        # 将参数register与caller-id-in-from的属性设置为false或者true.
        if listParamNames[loop].strip() == 'register' or listParamNames[loop].strip() == 'caller-id-in-from':
            if listParamValues[loop].strip() == '0':     
                domParamNode.setAttribute('value', 'false')
            else:
                domParamNode.setAttribute('value', 'true')
        else:
            domParamNode.setAttribute('value', str(listParamValues[loop]).strip())
        domGatewayNode.appendChild(domParamNode)
        
    domIncludeNode = doc.createElement('include')
    domIncludeNode.appendChild(domGatewayNode)
    doc.appendChild(domIncludeNode)
    
    seqCfgDir = conf_path.get_config_path()
    if -1 == seqCfgDir:
        file_info.print_file_info('Search the path of FreeSWITCH FAIL.')
        return -1
    if seqCfgDir[-1] != '/':
        seqCfgDir = seqCfgDir + '/'
    seqCfgDir = seqCfgDir + 'sip_profiles/external/'
    if os.path.exists(seqCfgDir) is False:
        os.makedirs(seqCfgDir)
    
    seqCfgPath = seqCfgDir + str(ulGatewayID) + '.xml'
    lRet = dom_to_xml.dom_to_xml(seqCfgPath, doc)
    if -1 == lRet:
        file_info.print_file_info('Generate route Configuration FAIL, lRet is %d' % lRet)
        return -1
    lRet = dom_to_xml.del_xml_head(seqCfgPath)
    if -1 == lRet:
        file_info.print_file_info('Generate route Configuration FAIL, lRet is %d' % lRet)
        return -1

    file_info.print_file_info('Generate route %d Configuration SUCC.' % ulGatewayID)
    return 1
    
def del_route(ulGatewayID):
    '''
    @todo： 网关ID
    '''
    if str(ulGatewayID).strip() == '':
        file_info.print_file_info('Delete route configuration FAIL,ulGatewayID is %s' % str(ulGatewayID))
        return -1
    seqCfgDir = db_config.get_db_param()['fs_config_path']
    if seqCfgDir[-1] != '/':
        seqCfgDir = seqCfgDir + '/'
    
    seqCfgDir = seqCfgDir + 'sip_profiles/external/'
    if os.path.exists(seqCfgDir) is False:
        os.makedirs(seqCfgDir)
    
    seqCfgPath = seqCfgDir + str(ulGatewayID) + '.xml'
    
    if os.path.exists(seqCfgPath):
        os.system("rm %s" % (seqCfgPath))

    file_info.print_file_info('Delete route %d Configuration SUCC.' % ulGatewayID)
    return 1
