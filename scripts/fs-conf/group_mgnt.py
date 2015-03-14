# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 11th,2015
@todo: operation of group
'''

import db_conn
import file_info
import sip_maker
import db_config
from __builtin__ import str

def add_group(ulGroupID):
    '''
    @param grp_id: 座席组id
    @todo: 增加一个座席组
    '''
    if str(ulGroupID) == '':
        file_info.get_cur_runtime_info('ulGroupID is %s' % str(ulGroupID))
        return -1
    
    # 获取客户id
    ulCustomerID = get_customer_id(ulGroupID)
    if -1 == ulCustomerID:
        file_info.get_cur_runtime_info('ulCustomerID is %d' % ulCustomerID)
        return -1
    # 获取所有座席
    results = get_all_agent(ulGroupID)
    seqCfgPath = db_config.get_db_param()['fs_config_path']
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqMgntPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    for loop in range(len(results)):
        agent_path = seqCfgPath + 'directory/' + str(ulCustomerID) + '/' + results[loop][0] + '.xml'
        lRet = sip_maker.make_sip(results[loop][0], ulCustomerID, agent_path)
        if -1 == lRet:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
    
    # 将座席组配置信息添加至文件对应位置
    listTextList = open(seqMgntPath, 'r').readlines()
    if listTextList == []:
        file_info.get_cur_runtime_info(listTextList)
        return -1
    seqText = '   <group name=\"%s-%s\">\n    <users>\n' % (str(ulCustomerID), str(ulGroupID))
    seqOneText = '   <group name=\"%s-%s\">\n' % (str(ulCustomerID), str(ulGroupID))
    
    # 如果说配置文件中有关于该座席组的信息，则直接返回
    if seqOneText in listTextList:
        return -1
    for loop in range(len(results)):
        seqText = seqText + ('     <user id=\"%s\" type=\"pointer\"/>\n' % results[loop][0])
    seqText = seqText + '    </users>\n   </group>\n'
    ulIndex = listTextList.index('   </group>\n')
    listTextList.insert(ulIndex + 1, seqText)
    open(seqMgntPath, 'w').writelines(listTextList)
    return 1
    
    
def get_customer_id(ulGroupID):
    '''
    @param grp_id: 座席组id
    @todo: 获取客户id
    '''
    global CONN
    if str(ulGroupID).strip() == '':
        file_info.get_cur_runtime_info('ulGroupID is %s' % str(ulGroupID))
        return -1
    
    #查找group id为`grp_id`的客户id
    seqSQLCmd = 'SELECT CONVERT(tbl_group.customer_id, CHAR(10)) AS customer_id FROM tbl_group WHERE tbl_group.id = %d' % ulGroupID
    file_info.get_cur_runtime_info(seqSQLCmd)
    lRet = db_conn.connect_db()
    if -1 == lRet:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        file_info.get_cur_runtime_info('The database connection does not exist.')
        return -1
    cursor.execute(seqSQLCmd)
    results = cursor.fetchall()
    if len(results) == 0:
        file_info.get_cur_runtime_info('len(results) is %d' % len(results))
        return -1
    db_conn.CONN.close()
    file_info.get_cur_runtime_info(results)
    return results[0][0]

def get_all_agent(ulGroupID):
    '''
    @param group_id: 座席组id
    @todo: 获取座席组下所有座席
    '''
    global CONN
    if str(ulGroupID).strip() == '':
        file_info.get_cur_runtime_info('ulGroupID is %s' % str(ulGroupID))
        return -1
    
    # 获取所有座席的座席组为`group1_id`或者`group2_id`的座席id
    seqSQLCmd = 'SELECT CONVERT(tbl_agent.id ,CHAR(10)) AS id FROM tbl_agent WHERE tbl_agent.group1_id = %d OR tbl_agent.group2_id = %d' % (ulGroupID,ulGroupID)
    file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
    lRet = db_conn.connect_db()
    if -1 == lRet:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        file_info.get_cur_runtime_info('The database connection does not exist.')
        return -1
    cursor.execute(seqSQLCmd)
    results = cursor.fetchall()
    if len(results) == 0:
        file_info.get_cur_runtime_info('len(results) is %d' % len(results))
        return -1
    db_conn.CONN.close()
    file_info.get_cur_runtime_info(results)
    return results

def del_group(ulGroupID, ulCustomerID):
    '''
    @param grp_id: 座席组id
    @param customer_id: 客户id
    @todo: 将一个座席组从一个客户下删除
    '''
    if str(ulGroupID).strip() == '':
        file_info.get_cur_runtime_info('ulGroupID is %s' % str(ulGroupID))
        return -1
    if str(ulCustomerID).strip() == '':
        file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
        return -1
    seqCfgPath = db_config.get_db_param()['fs_config_path']
    if -1 == seqCfgPath:
        file_info.get_cur_runtime_info('seqCfgPath is %d' % seqCfgPath)
        return -1
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqMgntPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    
    # 将座席组信息从配置文件干掉
    listTextList = open(seqMgntPath, 'r').readlines()
    if listTextList == []:
        file_info.get_cur_runtime_info(listTextList)
        return -1
    ulIndex = listTextList.index('   <group name=\"%s-%s\">\n' % (str(ulCustomerID), str(ulGroupID)))
    if ulIndex < 0:
        file_info.get_cur_runtime_info('ulIndex is %d' % ulIndex)
        return -1
    ulEndIndex = ulIndex
    for loop in range(ulIndex, len(listTextList)):
        if listTextList[loop] == '   </group>\n':
            ulEndIndex = loop
            break
    del listTextList[ulIndex : ulEndIndex + 1]
    open(seqMgntPath, 'w').writelines(listTextList)
    return 1
    
def modify_group_name(ulOldGroupID, ulNewGroupID):
    '''
    @param old_grp_id: 原来的座席组id 
    @param new_grp_id: 新的座席id
    @todo:  从配置文件修改座席id
    '''
    if str(ulOldGroupID).strip() == '':
        file_info.get_cur_runtime_info('ulOldGroupID is %s' % str(ulOldGroupID))
        return -1
    if str(ulNewGroupID).strip() == '':
        file_info.get_cur_runtime_info('ulNewGroupID is %s' % str(ulNewGroupID))
        return -1
    ulCustomerID = get_customer_id(ulNewGroupID)
    if -1 == ulCustomerID:
        file_info.get_cur_runtime_info('ulCustomerID is %d' % ulCustomerID)
        return -1
    
    seqCfgPath = db_config.get_db_param()['fs_config_path']
    if -1 == seqCfgPath:
        file_info.get_cur_runtime_info('seqCfgPath is %d' % seqCfgPath)
        return -1
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqMgntPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    
    # 找到group的id信息并修改
    listTextList = open(seqMgntPath, 'r').readlines()
    if listTextList == []:
        file_info.get_cur_runtime_info(listTextList)
        return -1
    ulIndex = listTextList.index('   <group name=\"%s-%s\">\n' % (str(ulCustomerID), str(ulOldGroupID)))
    if ulIndex < 0:
        file_info.get_cur_runtime_info('ulIndex is %d' % ulIndex)
        return
    listTextList[ulIndex] = '   <group name=\"%s-%s\">\n' % (str(ulCustomerID), str(ulNewGroupID))
    open(seqMgntPath, 'w').writelines(listTextList)
    return 1
