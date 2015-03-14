# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 9rd,2015
@todo: generate the directories and management xml files
'''
from xml.dom.minidom  import Document
import db_conn
import file_info
import db_config
import os
import dom_to_xml
from __builtin__ import str
from _ast import Str

def phone_route():
    '''
    @todo: ������������
    '''
    global CONN
    lRet = db_conn.connect_db()
    if -1 == lRet:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
  
    # ���ҳ������м���id
    seqSQLCmd = 'SELECT DISTINCT CONVERT(id, CHAR(10)) AS id FROM tbl_relaygrp'
    file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
    cursor = db_conn.CONN.cursor()
    cursor.execute(seqSQLCmd)
    results = cursor.fetchall()
        
    file_info.get_cur_runtime_info(results)
    for loop in range(len(results)):
        make_route(int(results[loop][0]))
    
    return 1
    
def get_route_param(id):
    '''
    @param id: ����id
    @todo: ����һ��·�ɱ�
    '''
    global CONN 
    lRet = db_conn.connect_db()
    if -1 == lRet:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
    
    # ���ҳ����е����ز���ֵ
    seqSQLCmd = 'SELECT name,username,password,realm,form_user,form_domain,extension,proxy,reg_proxy,expire_secs,CONVERT(register, CHAR(10)) AS register,reg_transport,CONVERT(retry_secs, CHAR(20)) AS retry_secs, CONVERT(cid_in_from,CHAR(20)) AS cid_in_from,contact_params, CONVERT(exten_in_contact, CHAR(20)) AS exten_in_contact,CONVERT(ping, CHAR(20)) AS ping FROM tbl_relaygrp WHERE id=%d' % (id)
    file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
    cursor = db_conn.CONN.cursor()
    cursor.execute(seqSQLCmd)
    results = cursor.fetchall()
    file_info.get_cur_runtime_info(results)
    db_conn.CONN.close()
    return results

def make_route(ulGatewayID):
    '''
    @param id: ����id
    @param doc: �ĵ�����
    @todo: ����һ����������DOM
    '''
    doc = Document()
    results = get_route_param(ulGatewayID)  
    if -1 == results:
        file_info.get_cur_runtime_info('results is %d' % results)
        return -1
    domGatewayNode = doc.createElement('gateway')
    domGatewayNode.setAttribute('name', results[0][0])
    
    listParamNames  = ['username', 'realm', 'from-user', 'from-domain', 'password', 'extension', 'proxy', 'register-proxy', 'expire-seconds', 'register', 'reg_transport', 'retry_secs', 'caller-id-in-from', 'contact_params','exten_in_contact', 'ping']
    listParamValues = [results[0][1], results[0][3], results[0][4], results[0][5], results[0][2], results[0][6], results[0][7], results[0][9], results[0][8], results[0][10], results[0][11], results[0][12], results[0][13], results[0][14], results[0][15], results[0][16]]
    
    for loop in range(len(listParamNames)):
        domParamNode = doc.createElement('param')
        domParamNode.setAttribute('name', listParamNames[loop].strip())
        
        # ����'register'��'caller-id-in-from'�Ĳ���ֵʹ�ò���ֵȥ��ʾ
        if listParamNames[loop].strip() == 'register' or listParamNames[loop].strip() == 'caller-id-in-from':
            if listParamValues[loop].strip() == '0':     
                domParamNode.setAttribute('value', 'false')
            else:
                domParamNode.setAttribute('value', 'true')
        else:
            domParamNode.setAttribute('value', listParamValues[loop].strip())
        
        domGatewayNode.appendChild(domParamNode)
        
    domIncludeNode = doc.createElement('include')
    domIncludeNode.appendChild(domGatewayNode)
    doc.appendChild(domIncludeNode)
    
    seqCfgDir = db_config.get_db_param()['cfg_path']
    if -1 == seqCfgDir:
        file_info.get_cur_runtime_info('seqCfgDir is %d' % seqCfgDir)
        return -1
    if seqCfgDir[-1] != '/':
        seqCfgDir = seqCfgDir + '/'
    
    seqCfgDir = seqCfgDir + 'sip_profiles/external/'
    if os.path.exists(seqCfgDir) is False:
        os.makedirs(seqCfgDir)
    
    seqCfgPath = seqCfgDir + str(ulGatewayID) + '.xml'
    lRet = dom_to_xml.dom_to_pretty_xml(seqCfgPath, doc)
    if -1 == lRet:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
    lRet = dom_to_xml.del_xml_head(seqCfgPath)
    if -1 == lRet:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
    
    return 1
    
def del_route(ulGatewayID):
    if str(ulGatewayID).strip() == '':
        file_info.get_cur_runtime_info('ulGatewayID is %s' % str(ulGatewayID))
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
        
    return 1
    
make_route(1)
    