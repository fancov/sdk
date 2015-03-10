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
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is', ulGroupID
        return None
    
    # 获取客户id
    ulCustomerID = get_customer_id(ulGroupID)
    # 获取所有座席
    results = get_all_agent(ulGroupID)
    seqCfgPath = db_config.get_db_param()['cfg_path']
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqMgntPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    for loop in range(len(results)):
        agent_path = seqCfgPath + 'directory/' + str(ulCustomerID) + '/' + results[loop][0] + '.xml'
        sip_maker.make_sip(results[loop][0], ulCustomerID, agent_path)
    
    # 将座席组配置信息添加至文件对应位置
    listTextList = open(seqMgntPath, 'r').readlines()
    if listTextList == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'text_list is', listTextList
        return
    seqText = '   <group name=\"%s-%s\">\n    <users>\n' % (str(ulCustomerID), str(ulGroupID))
    seqOneText = '   <group name=\"%s-%s\">\n' % (str(ulCustomerID), str(ulGroupID))
    
    # 如果说配置文件中有关于该座席组的信息，则直接返回
    if seqOneText in listTextList:
        return
    for loop in range(len(results)):
        seqText = seqText + ('     <user id=\"%s\" type=\"pointer\"/>\n' % results[loop][0])
    seqText = seqText + '    </users>\n   </group>\n'
    ulIndex = listTextList.index('   </group>\n')
    listTextList.insert(ulIndex + 1, seqText)
    open(seqMgntPath, 'w').writelines(listTextList)
    
    
def get_customer_id(ulGroupID):
    '''
    @param grp_id: 座席组id
    @todo: 获取客户id
    '''
    global CONN
    if str(ulGroupID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is', ulGroupID
        return None
    
    #查找group id为`grp_id`的客户id
    seqSQLCmd = 'SELECT CONVERT(tbl_group.customer_id, CHAR(10)) AS customer_id FROM tbl_group WHERE tbl_group.id = %d' % ulGroupID
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',seqSQLCmd
    db_conn.connect_db()
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'The database connection does not exist.'
        return
    cursor.execute(seqSQLCmd)
    results = cursor.fetchall()
    if len(results) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'len(results) is', len(results)
        return
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'results is:',results
    return results[0][0]

def get_all_agent(ulGroupID):
    '''
    @param group_id: 座席组id
    @todo: 获取座席组下所有座席
    '''
    global CONN
    if str(ulGroupID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'group_id is:',ulGroupID
        return None
    
    # 获取所有座席的座席组为`group1_id`或者`group2_id`的座席id
    seqSQLCmd = 'SELECT CONVERT(tbl_agent.id ,CHAR(10)) AS id FROM tbl_agent WHERE tbl_agent.group1_id = %d OR tbl_agent.group2_id = %d' % (ulGroupID,ulGroupID)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',seqSQLCmd
    db_conn.connect_db()
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'The database connection does not exist.'
        return
    cursor.execute(seqSQLCmd)
    results = cursor.fetchall()
    if len(results) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'len(results) is', len(results)
        return
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'results is:', results
    return results

def del_group(ulGroupID, ulCustomerID):
    '''
    @param grp_id: 座席组id
    @param customer_id: 客户id
    @todo: 将一个座席组从一个客户下删除
    '''
    if str(ulGroupID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is',ulGroupID
        return None
    if str(ulCustomerID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is',ulCustomerID
        return None
    seqCfgPath = db_config.get_db_param()['cfg_path']
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqMgntPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    
    # 将座席组信息从配置文件干掉
    listTextList = open(seqMgntPath, 'r').readlines()
    if listTextList == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'text_list is', listTextList
        return
    ulIndex = listTextList.index('   <group name=\"%s-%s\">\n' % (str(ulCustomerID), str(ulGroupID)))
    if ulIndex < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'index is', ulIndex
        return 
    ulEndIndex = ulIndex
    for loop in range(ulIndex, len(listTextList)):
        if listTextList[loop] == '   </group>\n':
            ulEndIndex = loop
            break
    del listTextList[ulIndex : ulEndIndex + 1]
    open(seqMgntPath, 'w').writelines(listTextList)
    
def modify_group_name(ulOldGroupID, ulNewGroupID):
    '''
    @param old_grp_id: 原来的座席组id 
    @param new_grp_id: 新的座席id
    @todo:  从配置文件修改座席id
    '''
    if str(ulOldGroupID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'old_grp_id is',ulOldGroupID
        return None
    if str(ulNewGroupID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'new_grp_id is',ulNewGroupID
        return None
    ulCustomerID = get_customer_id(ulNewGroupID)
    seqCfgPath = db_config.get_db_param()['cfg_path']
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    seqMgntPath = seqCfgPath + 'directory/' + ulCustomerID + '.xml'
    
    # 找到group的id信息并修改
    listTextList = open(seqMgntPath, 'r').readlines()
    if listTextList == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'text_list is', listTextList
        return
    ulIndex = listTextList.index('   <group name=\"%s-%s\">\n' % (str(ulCustomerID), str(ulOldGroupID)))
    if ulIndex < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'index is', ulIndex
        return
    listTextList[ulIndex] = '   <group name=\"%s-%s\">\n' % (str(ulCustomerID), str(ulNewGroupID))
    open(seqMgntPath, 'w').writelines(listTextList)
