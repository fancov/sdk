# -*- encoding=utf-8 -*-
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

def add_group(grp_id):
    '''
    @param grp_id: 座席组id
    @todo: 增加一个座席组
    '''
    if str(grp_id) == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is', grp_id
        return None
    
    # 获取客户id
    customer_id = get_customer_id(grp_id)
    # 获取所有座席
    results = get_all_agent(grp_id)
    cfg_path = db_config.get_db_param()['cfg_path']
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
    mgnt_path = cfg_path + 'directory/' + str(customer_id) + '.xml'
    for loop in range(len(results)):
        agent_path = cfg_path + 'directory/' + str(customer_id) + '/' + results[loop][0] + '.xml'
        sip_maker.make_sip(results[loop][0], customer_id, agent_path)
    
    # 将座席组配置信息添加至文件对应位置
    text_list = open(mgnt_path, 'r').readlines()
    if text_list == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'text_list is', text_list
        return
    text = '   <group name=\"%s-%s\">\n    <users>\n' % (str(customer_id), str(grp_id))
    one_text = '   <group name=\"%s-%s\">\n' % (str(customer_id), str(grp_id))
    
    # 如果说配置文件中有关于该座席组的信息，则直接返回
    if one_text in text_list:
        return
    for loop in range(len(results)):
        text = text + ('     <user id=\"%s\" type=\"pointer\"/>\n' % results[loop][0])
    text = text + '    </users>\n   </group>\n'
    index = text_list.index('   </group>\n')
    text_list.insert(index + 1, text)
    open(mgnt_path, 'w').writelines(text_list)
    
    
def get_customer_id(grp_id):
    '''
    @param grp_id: 座席组id
    @todo: 获取客户id
    '''
    global CONN
    if str(grp_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is', grp_id
        return None
    
    #查找group id为`grp_id`的客户id
    sql_cmd = 'SELECT CONVERT(tbl_group.customer_id, CHAR(10)) AS customer_id FROM tbl_group WHERE tbl_group.id = %d' % grp_id
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',sql_cmd
    db_conn.connect_db()
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'The database connection does not exist.'
        return
    cursor.execute(sql_cmd)
    results = cursor.fetchall()
    if len(results) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'len(results) is', len(results)
        return
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'results is:',results
    return results[0][0]

def get_all_agent(group_id):
    '''
    @param group_id: 座席组id
    @todo: 获取座席组下所有座席
    '''
    global CONN
    if str(group_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'group_id is:',group_id
        return None
    
    # 获取所有座席的座席组为`group1_id`或者`group2_id`的座席id
    sql_cmd = 'SELECT CONVERT(tbl_agent.id ,CHAR(10)) AS id FROM tbl_agent WHERE tbl_agent.group1_id = %d OR tbl_agent.group2_id = %d' % (group_id,group_id)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',sql_cmd
    db_conn.connect_db()
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'The database connection does not exist.'
        return
    cursor.execute(sql_cmd)
    results = cursor.fetchall()
    if len(results) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'len(results) is', len(results)
        return
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'results is:', results
    return results

def del_group(grp_id, customer_id):
    '''
    @param grp_id: 座席组id
    @param customer_id: 客户id
    @todo: 将一个座席组从一个客户下删除
    '''
    if str(grp_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is',grp_id
        return None
    if str(customer_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is',customer_id
        return None
    cfg_path = db_config.get_db_param()['cfg_path']
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
    mgnt_path = cfg_path + 'directory/' + str(customer_id) + '.xml'
    
    # 将座席组信息从配置文件干掉
    text_list = open(mgnt_path, 'r').readlines()
    if text_list == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'text_list is', text_list
        return
    index = text_list.index('   <group name=\"%s-%s\">\n' % (str(customer_id), str(grp_id)))
    if index < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'index is', index
        return 
    end_index = index
    for loop in range(index, len(text_list)):
        if text_list[loop] == '   </group>\n':
            end_index = loop
            break
    del text_list[index : end_index + 1]
    open(mgnt_path, 'w').writelines(text_list)
    
def modify_group_name(old_grp_id, new_grp_id):
    '''
    @param old_grp_id: 原来的座席组id 
    @param new_grp_id: 新的座席id
    @todo:  从配置文件修改座席id
    '''
    if str(old_grp_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'old_grp_id is',old_grp_id
        return None
    if str(new_grp_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'new_grp_id is',new_grp_id
        return None
    customer_id = get_customer_id(new_grp_id)
    cfg_path = db_config.get_db_param()['cfg_path']
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
    mgnt_path = cfg_path + 'directory/' + customer_id + '.xml'
    
    # 找到group的id信息并修改
    text_list = open(mgnt_path, 'r').readlines()
    if text_list == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'text_list is', text_list
        return
    index = text_list.index('   <group name=\"%s-%s\">\n' % (str(customer_id), str(old_grp_id)))
    if index < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'index is', index
        return
    text_list[index] = '   <group name=\"%s-%s\">\n' % (str(customer_id), str(new_grp_id))
    open(mgnt_path, 'w').writelines(text_list)
