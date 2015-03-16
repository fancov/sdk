# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 10th,2015
@todo: operation of sip account
'''

import os
import sip_maker
import db_conn
import file_info
import db_config

def add_sip(ulSIPID):
    '''
    @param: sip_id  sip�˻�id
    @todo: ���һ��sip�˻�
    '''
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s', str(ulSIPID))
        return -1
    
    #��ȡsip�˻���group id
    try:
        ulGroupID = get_grp_id(ulSIPID)
    except Exception, err:
        file_info.get_cur_runtime_info('Catch Exception:%s' % str(err))
        return -1
    else:
        if ulGroupID == -1:
            file_info.get_cur_runtime_info('ulGroupID is %s' % str(ulGroupID))
            return -1
    
        for item in ulGroupID[0]:
            if str(item).strip() == '0':
                # ɾ��group idΪ0����
                del item
            else:
                # ��sip�˻���ӵ���ϯ��
                add_sip_to_group(ulSIPID, int(item))
        return 1
            

def get_grp_id(ulSIPID):
    '''
    @param: sip_id sip�˻�id
    @todo: ��ȡsip�˻����ڵ���ϯ��id�б�
    '''
    global CONN 
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    
    try:
        lRet = db_conn.connect_db()
    except Exception,err:
        file_info.get_cur_runtime_info('Catch Exception:%s.' % str(err))
        return -1
    else:
        if -1 == lRet:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
    
        # 查找sip属于那个座席
        seqSQLCmd = 'SELECT CONVERT(tbl_agent.group1_id,CHAR(10)) AS group1_id,CONVERT(tbl_agent.group2_id,CHAR(10)) AS group2_id FROM tbl_agent WHERE tbl_agent.sip_id = %d' % (ulSIPID)
        file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
    
        cursor = db_conn.CONN.cursor()
        if cursor is None:
            file_info.get_cur_runtime_info('The database connection does not exist.')
            return -1
        cursor.execute(seqSQLCmd)
        ulGroupID = cursor.fetchall()
        if len(ulGroupID) == 0:
            file_info.get_cur_runtime_info('len(ulGroupID) is %d' % len(ulGroupID))
            return -1
        db_conn.CONN.close()
        file_info.get_cur_runtime_info('ulGroupID is %s' % str(ulGroupID))
    
    #������ϯ��id�б���Ϊһ��sip�˻��ܿ������ڶ���˻�
        return ulGroupID

def get_customer_id(ulSIPID, ulGroupID):
    '''
    @param: sip_id sip�˻�id; grp_id sip�˻����ڵ���ϯ��id
    @todo: ����sip��id��ȡ�������ͻ���id
    '''
    
    global CONN
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    if str(ulGroupID).strip() == '':
        file_info.get_cur_runtime_info('ulGroupID is %s' % str(ulGroupID))
        return -1
    
    try:
        lRet = db_conn.connect_db()
    except Exception, err:
        file_info.get_cur_runtime_info('Catch Exception:%s' % str(err))
        return -1
    else:
        if lRet == -1:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
        # ��ȡ��ϯ��idΪgrp_id�����Ŀͻ�id
        seqSQLCmd = 'SELECT CONVERT(tbl_group.customer_id,CHAR(10)) AS customer_id FROM tbl_group WHERE tbl_group.id = %s' % (ulGroupID)
        file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
        cursor = db_conn.CONN.cursor()
        if cursor is None:
            file_info.get_cur_runtime_info('The database connection does not exist.')
            return -1
        cursor.execute(seqSQLCmd)
        ulCustomerID = cursor.fetchall()
        db_conn.CONN.close()
        if len(ulCustomerID) == 0:
            file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
            return -1
        file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
        return ulCustomerID

def get_agent_by_sip(ulSIPID):
    '''
    @param: sip_id sip�˻�id
    @todo: ��ȡsip�˻���Ӧ����ϯ��id
    '''
    global CONN
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    lRet = db_conn.connect_db()
    if -1 == lRet:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
    
    # ��ȡsip id��sip_id��agent id
    seqSQLCmd = 'SELECT CONVERT(id, CHAR(10)) AS id FROM tbl_agent WHERE tbl_agent.sip_id = %d' % (ulSIPID)
    file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        file_info.get_cur_runtime_info('The database connection does not exist.')
        return -1
    cursor.execute(seqSQLCmd)
    result = cursor.fetchall()
    if len(result) == 0:
        file_info.get_cur_runtime_info('len(result) is %d' % len(result))
        return -1
    db_conn.CONN.close()
    file_info.get_cur_runtime_info(result)
    return result

def add_sip_to_group(ulSIPID, ulGroupID):
    '''
    @param: sip_id sip�˻�id; grp_id sip�˻�������ϯ��id
    @todo: ��sip�˻���ӵ�group��
    '''
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    if str(ulGroupID).strip() == '':
        file_info.get_cur_runtime_info('ulGroupID is %s' % str(ulGroupID))
        return -1
    result = get_customer_id(ulSIPID, ulGroupID)
    if result is None or result == -1:
        file_info.get_cur_runtime_info(result)
        return -1
    file_info.get_cur_runtime_info(result)
    
    #��ȡ�ͻ�id
    ulCustomerID = result[0][0]
    
    seqCfgPath = db_config.get_db_param()['fs_config_path']
    if seqCfgPath == -1:
        file_info.get_cur_runtime_info('seqCfgPath is %s' % str(seqCfgPath))
        return -1
    #������'/'��β��·���ַ���
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    file_info.get_cur_runtime_info('seqCfgPath is %s' % str(seqCfgPath))
    
    # �ͻ������ļ�����·������:����customer id��1����ô�ͻ������ļ��� %cfg_path%/directory/1.xml
    seqCustomerDir = seqCfgPath + 'directory/' + ulCustomerID + '/'
    if os.path.exists(seqCustomerDir) is False:
        os.makedirs(seqCustomerDir)
        
    seqCustomerPath = seqCfgPath + 'directory/' + ulCustomerID + '.xml'
    seqSipPath = seqCustomerDir + str(ulSIPID) + '.xml' 
    
    file_info.get_cur_runtime_info('seqCustomerPath is %s' % seqCustomerPath)
    
    if os.path.exists(seqCustomerPath) is False:
        sip_maker.make_sip(ulSIPID, ulCustomerID, seqSipPath)
        return 1
    
    seqMgntText = open(seqCustomerPath, 'r').readlines()
    if seqMgntText == []:
        file_info.get_cur_runtime_info('mgnt_text is empty.')
        return -1
    file_info.get_cur_runtime_info('seqMgntText is %s' % seqMgntText)
    # �����µ�sip�����ַ��������
    seqAddText = '     <user id=\"%s\" type=\"pointer\"/>\n' % (ulSIPID)
    if seqAddText in seqMgntText:
        return -1
    file_info.get_cur_runtime_info('seqAddText is %s' % seqAddText)
    try:
        ulMgntIndex = seqMgntText.index('   <group name=\"%s-%s\">\n' % (ulCustomerID, ulGroupID))
        if ulMgntIndex < 0:
            file_info.get_cur_runtime_info('mgnt_text does not exist.')
            return  -1
        seqMgntText.insert(ulMgntIndex + 2, seqAddText)
        open(seqCustomerPath, 'w').writelines(seqMgntText)
        # ����sip�˻�
        lRet = sip_maker.make_sip(ulSIPID, ulCustomerID, seqSipPath)
        if lRet == -1:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
        return 1
    except Exception as e:
        file_info.get_cur_runtime_info('Catch Exception:%s' % str(e))
        return -1


def del_sip(ulSIPID, ulCustomerID):
    '''
    @todo: 删除一个sip账户
    '''
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    if str(ulCustomerID).strip() == '':
        file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
        return -1
    
    # 找到配置文件根目录
    seqCfgPath = db_config.get_db_param()['fs_config_path']
    if -1 == seqCfgPath:
        file_info.get_cur_runtime_info('seqCfgPath is %d' % seqCfgPath)
        return -1
    
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
        
    # 找到sip账户配置文件
    seqSIPPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '/' + str(ulSIPID) + '.xml'
    if os.path.exists(seqSIPPath):
        os.remove(seqSIPPath)
    
    # 找到sip账户的管理文件
    seqMgntPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'  
    if os.path.exists(seqMgntPath) is False:
        file_info.get_cur_runtime_info('The mgnt file does not exist.')
        return -1
    
    # 读取文件
    listMgntText = open(seqMgntPath,'r').readlines()
    if listMgntText == []:
        file_info.get_cur_runtime_info(listMgntText)
        return -1
    
    # 找到该sip账户的信息
    ulIndex = listMgntText.index('     <user id=\"%d\" type=\"pointer\"/>\n' % ulSIPID)
    if ulIndex < 0:
        file_info.get_cur_runtime_info('The index does not exist')
        return -1
    
    del listMgntText[ulIndex]
    open(seqMgntPath, 'w').writelines(listMgntText)
    return 1

def change_agent_group(ulSIPID, ulNewGroupID):
    '''
    @param sip_id: sip�˻�id
    @param new_grp_id: �µ���ϯ��id
    @todo:��sip�˻����ĵ�����һ��ϯ��
    '''
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    if str(ulNewGroupID).strip() == '':
        file_info.get_cur_runtime_info('ulNewGroupID is %s' % str(ulNewGroupID))
        return -1
    
    # ��ȡ����ϯ��id
    result = get_agent_by_sip(ulSIPID)
    if -1 == result:
        file_info.get_cur_runtime_info(result)
        return -1
    file_info.get_cur_runtime_info(result)
    
    #��ȡ��customer id
    ulCustomerID = get_customer_id(ulSIPID, ulNewGroupID)[0][0]
    if -1 == ulCustomerID:
        file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
        return -1
    ulAgentID = result[0][0]
    seqCfgPath = db_config.get_db_param()['fs_config_path']
    if -1 == seqCfgPath:
        file_info.get_cur_runtime_info('seqCfgPath is %d' % seqCfgPath)
        return -1
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqMgntPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    listText = open(seqMgntPath, 'r').readlines()
    if listText == []:
        file_info.get_cur_runtime_info(listText)
        return -1
    
    seqDelText = '     <user id=\"%s\" type=\"pointer\"/>\n' % (ulAgentID)
    while True:
        if seqDelText in listText:
            ulIndex = listText.index(seqDelText)
            del listText[ulIndex]
        else:
            break
    
    seqGroupText = '   <group name=\"%s-%s\">\n' % (ulCustomerID, ulNewGroupID)
    if seqGroupText in listText:
        ulIndex = listText.index(seqGroupText)
        listText.insert(ulIndex + 2, seqDelText)
    else:
        ulIndex = listText.index('   </group>\n')
        seqAddText = '   <group name=\"%s-%s\">\n    <users>\n     <user id="%s" type="pointer"/>\n    </users>\n   </group>\n' % (ulCustomerID, ulNewGroupID, ulAgentID)
        listText.insert(ulIndex + 1, seqAddText)
    open(seqMgntPath, 'w').writelines(listText)
    return 1
    
def modify_param_value(ulSIPID, seqParamName, seqParamValue):
    '''
    @param sip_id: sip�˻�id
    @param param_name: ��������
    @param param_value: ����ֵ
    @todo: �޸Ĳ�����ֵ
    '''
    if str(ulSIPID).strip() == '':
        file_info.get_cur_runtime_info('ulSIPID is %s' % str(ulSIPID))
        return -1
    if seqParamName.strip() == '':
        file_info.get_cur_runtime_info('seqParamName is %s' % seqParamName)
        return -1
    if seqParamValue.strip() == '':
        file_info.get_cur_runtime_info('seqParamValue is %s' % seqParamValue)
        return -1
    
    # ��ȡ��ϯid
    result = get_agent_by_sip(ulSIPID)
    if -1 == result:
        file_info.get_cur_runtime_info('result is %d' % result)
        return -1
    ulAgentID = result[0][0]
    
    # ��ȡ��ϯ��id
    ulIDValue = get_grp_id(ulSIPID)
    if -1 == ulIDValue:
        file_info.get_cur_runtime_info('ulIDValue is %d' % ulIDValue)
        return -1
    ulGroupID = '0'
    if ulIDValue[0][0].strip() == '0':
        ulGroupID = ulIDValue[1][0]
    else:
        ulGroupID = ulIDValue[0][0]
        
    _res = get_customer_id(ulSIPID, ulGroupID)
    if -1 == _res:
        file_info.get_cur_runtime_info('_res is %d' % _res)
        return -1
    ulCustomerID = _res[0][0]
    seqCfgPath = db_config.get_db_param()['fs_config_path']
    if -1 == seqCfgPath:
        file_info.get_cur_runtime_info('seqCfgPath is %d' % seqCfgPath)
        return -1
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqSIPPath = seqCfgPath + 'directory/' + ulCustomerID + '/' + ulAgentID + '.xml'
    listText = open(seqSIPPath, 'r').readlines()
    if listText == []:
        file_info.get_cur_runtime_info(listText)
        return -1
    print listText
    for loop in range(len(listText)):
        if ('   <param name=\"%s\"' % seqParamName) in listText[loop]:
            listText[loop] = '   <param name=\"%s\" value=\"%s\"/>\n' % (seqParamName, seqParamValue)
    open(seqSIPPath, 'w').writelines(listText)
    return 1
    
    