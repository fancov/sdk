# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: generate the sip account xml files
'''

from xml.dom.minidom import Document
import dom_to_xml
import os
import db_conn
import file_info
import db_config
import sip_maker

def generate_sip_account(seqUserID, ulCustomerID):
    '''
    @param sip_id: sip�˻�id
    @param customer_id: sip�˻������ͻ�id
    @todo: generate sip account configuration
    '''
    if str(seqUserID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % seqUserID)
        return -1
    if str(ulCustomerID).strip() == '':
        file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
        return -1
    
    seqCfgPath = db_config.get_db_param()['fs_config_path']
    if seqCfgPath is None or seqCfgPath == '' or seqCfgPath == -1:
        file_info.get_cur_runtime_info('The param is invalid.')
        return -1
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    if os.path.exists(seqCfgPath) is False:
        os.makedirs(seqCfgPath) 
    seqSIPPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '/' + str(seqUserID) + '.xml'
    
    # 获取sip相关信息
    listResult = get_sipinfo_by_userid(seqUserID)
    if -1 == listResult:
        file_info.get_cur_runtime_info('listResult is %d' % listResult)
        return -1
    
    
    lRet = sip_maker.make_sip(listResult[0][0], ulCustomerID, seqSIPPath)
    if -1 == lRet:
        return -1
    return 1

def get_sipid_by_agent(ulAgentID):
    '''
    @todo: 根据坐席id获取sip账户id
    '''
    
    if str(ulAgentID).strip() == '':
        file_info.get_cur_runtime_info('ulAgentID is %s' % str(ulAgentID))
        return -1
    
    seqSQLCmd = 'SELECT sip_id FROM tbl_agent WHERE id=%d' % ulAgentID
    file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
    
    try:
        lRet = db_conn.connect_db()
    except Exception, err:
        file_info.get_cur_runtime_info('Catch Exception:%s' % str(err))
        return -1
    else:
        if lRet == -1:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
        
        cursor = db_conn.CONN.cursor()
        if cursor is None:
            file_info.get_cur_runtime_info('The database connection does not exist.')
            return -1
        
        cursor.execute(seqSQLCmd)
        listResult = cursor.fetchall()
        if len(listResult) == 0:
            file_info.get_cur_runtime_info('len(listResult) is %d' % len(listResult))
            return -1
        
        db_conn.CONN.close()
        
        ulSipID = int(listResult[0][0])
        
        return ulSipID
    
def get_sipinfo_by_userid(seqUserID):
    '''
    @todo: 根据userid获取sip账户信息
    '''
    
    if str(seqUserID).strip() == '':
        file_info.get_cur_runtime_info('seqUserID is %s' % seqUserID)
        return -1
    
    seqSQLCmd = 'SELECT customer_id, extension, dispname, authname, auth_password FROM tbl_sip WHERE userid = \'%s\' ' % seqUserID
    file_info.get_cur_runtime_info('seqSQLCmd is: %s' % seqSQLCmd)
    
    try:
        lRet = db_conn.connect_db()
    except Exception, err:
        file_info.get_cur_runtime_info('Catch Exception:%s' % str(err))
        return -1
    else:
        if -1 == lRet:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
        
        cursor = db_conn.CONN.cursor()
        if cursor is None:
            file_info.get_cur_runtime_info('The database connection does not exist.')
            return -1
        
        cursor.execute(seqSQLCmd)
        listResult = cursor.fetchall()
        
        if len(listResult) == 0:
            file_info.get_cur_runtime_info('len(listResult) is %d' % len(listResult))
            return -1
    
        file_info.get_cur_runtime_info(listResult)
        db_conn.CONN.close()
        
        print file_info
        return listResult