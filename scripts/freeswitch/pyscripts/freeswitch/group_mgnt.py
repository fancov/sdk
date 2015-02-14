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
    if str(grp_id) == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is', grp_id
        return None
    customer_id = get_customer_id(grp_id)
    results = get_all_agent(grp_id)
    cfg_path = db_config.get_db_param()['cfg_path']
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
    mgnt_path = cfg_path + 'directory/' + str(customer_id) + '.xml'
    for loop in range(len(results)):
        agent_path = cfg_path + 'directory/' + str(customer_id) + '/' + results[loop][0] + '.xml'
        sip_maker.make_sip(results[loop][0], customer_id, agent_path)
    text_list = open(mgnt_path, 'r').readlines()
    text = '   <group name=\"%s\">\n    <users>\n' % str(grp_id)
    for loop in range(len(results)):
        text = text + ('     <user id=\"%s\" type=\"pointer\"/>\n' % results[loop][0])
    text = text + '    </users>\n   </group>\n'
    index = text_list.index('   </group>\n')
    text_list.insert(index + 1, text)
    open(mgnt_path, 'w').writelines(text_list)
    
    
def get_customer_id(grp_id):
    global CONN
    if str(grp_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is', grp_id
        return None
    sql_cmd = 'select CONVERT(tbl_group.customer_id, CHAR(10)) as customer_id from tbl_group where tbl_group.id = %d' % grp_id
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',sql_cmd
    db_conn.connect_db()
    cursor = db_conn.CONN.cursor()
    cursor.execute(sql_cmd)
    results = cursor.fetchall()
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'results is:',results
    return results[0][0]

def get_all_agent(group_id):
    global CONN
    if str(group_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'group_id is:',group_id
        return None
    sql_cmd = 'select CONVERT(tbl_agent.id ,CHAR(10)) as id from tbl_agent where tbl_agent.group1_id = %d OR tbl_agent.group2_id = %d' % (group_id,group_id)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',sql_cmd
    db_conn.connect_db()
    cursor = db_conn.CONN.cursor()
    cursor.execute(sql_cmd)
    results = cursor.fetchall()
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'results is:', results
    return results

def del_group(grp_id, customer_id):
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
    text_list = open(mgnt_path, 'r').readlines()
    index = text_list.index('   <group name=\"%d\">\n' % grp_id)
    end_index = index
    for loop in range(index, len(text_list)):
        if text_list[loop] == '   </group>\n':
            end_index = loop
            break
    del text_list[index : end_index+1]
    open(mgnt_path, 'w').writelines(text_list)
    
def modify_group_name(old_grp_id, new_grp_id):
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
    text_list = open(mgnt_path, 'r').readlines()
    index = text_list.index('   <group name=\"%d\">\n' % old_grp_id)
    text_list[index] = '   <group name=\"%d\">\n' % new_grp_id
    open(mgnt_path, 'w').writelines(text_list)
    
    
    