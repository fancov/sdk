# -*- encoding=utf-8 -*-

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 10th,2015
@todo: operation of sip account
'''
import db_conn
import file_info
import db_conn
import db_config
import os
import sip_maker

def add_sip(sip_id):
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_is is', sip_id
        return None
    grp_id = get_grp_id(sip_id)
    
    for item in grp_id:
        if item.strip() == '0':
            del item
        else:
            add_sip_to_group(sip_id, int(item))
            

def get_grp_id(sip_id):
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is:', sip_id
        return
    global CONN
    db_conn.connect_db()
    sql_cmd = 'select CONVERT(tbl_agent.group1_id,CHAR(10)) as group1_id,CONVERT(tbl_agent.group2_id,CHAR(10)) as group2_id from tbl_agent where tbl_agent.sip_id = %d' % (sip_id)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',sql_cmd
    cursor = db_conn.CONN.cursor()
    cursor.execute(sql_cmd)
    grp_id = cursor.fetchall()
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'result:',grp_id
    return grp_id

def get_customer_id(sip_id, grp_id):
    global CONN
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is:', sip_id
        return None
    if str(grp_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is:', grp_id
        return None
    db_conn.connect_db()
    sql_cmd = 'select CONVERT(tbl_group.customer_id,CHAR(10)) as customer_id from tbl_group where tbl_group.id = %d' % (grp_id)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',sql_cmd
    cursor = db_conn.CONN.cursor()
    cursor.execute(sql_cmd)
    customer_id = cursor.fetchall()
    db_conn.CONN.close()
    if len(customer_id) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is:', customer_id
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is:', customer_id
    return customer_id

def get_agent_by_sip(sip_id):
    global CONN
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is',sip_id
        return None
    db_conn.connect_db()
    sql_cmd = 'select CONVERT(id, CHAR(10)) as id from tbl_agent where tbl_agent.sip_id = %d' % (sip_id)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:', sql_cmd
    cursor = db_conn.CONN.cursor()
    cursor.execute(sql_cmd)
    result = cursor.fetchall()
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'result:',result
    return result

def add_sip_to_group(sip_id, grp_id):
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is',sip_id
        return None
    if str(grp_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is',grp_id
    result = get_customer_id(sip_id, grp_id)
    if result is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'result is:', result
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'result is:', result
    customer_id = result[0][0]
    agent_id = get_agent_by_sip(sip_id)[0][0]
    
    cfg_path = db_config.get_db_param()['cfg_path']
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'cfg_path is:',cfg_path
    customer_path = cfg_path + 'directory/' + customer_id + '.xml'
    if os.path.exists(customer_path) is not True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_path is:',customer_path
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_path is:',customer_path
    
    sip_path = cfg_path + 'directory/' + customer_id + '/' + agent_id + '.xml'
    if os.path.exists(sip_path) is not True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_path is:', sip_path
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_path is:', sip_path
    
    mgnt_text = open(customer_path, 'r').readlines()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'mgnt_text is:', mgnt_text
    add_text = '<user id=\"%s\" type=\"pointer\"/>\n' % (agent_id)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'add_text is:', add_text
    try:
        mgnt_index = mgnt_text.index('   <group name=\"%s\">\n' % (grp_id))
        mgnt_text.insert(mgnt_index + 2, add_text)
        open(customer_path, 'w').writelines(mgnt_text)
        print mgnt_text
        sip_maker.make_sip(sip_id, customer_id, sip_path)
    except Exception as e:
        print str(e)

#agent_id sip账户对应的座席id,customer_id对应客户id    
def del_sip_from_group(agent_id, customer_id):
    if str(agent_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'agent_id is', agent_id
        return None
    if str(customer_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is',customer_id
        return None
    cfg_path = db_config.get_db_param()['cfg_path']
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
    sip_path = cfg_path + 'directory/' + str(customer_id) + '/' + str(agent_id) + '.xml'
    mgnt_path = cfg_path + 'directory/' + str(customer_id) + '.xml'
    os.remove(sip_path)
    mgnt_text = open(mgnt_path,'r').readlines()
    index = mgnt_text.index('     <user id=\"%d\" type=\"pointer\"/>\n')
    del mgnt_text[index]
    open(mgnt_path, 'w').writelines(mgnt_text)
   
def change_agent_group(sip_id, new_grp_id):
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is ',sip_id
        return None
    if str(new_grp_id).strip() == '':
        return file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'new_grp_id is', new_grp_id
        return None
    result = get_agent_by_sip(sip_id)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'result is:', result
    customer_id = get_customer_id(sip_id, new_grp_id)[0][0]
    agent_id = result[0][0]
    cfg_path = db_config.get_db_param()['cfg_path']
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
    mgnt_path = cfg_path + 'directory/' + str(customer_id) + '.xml'
    text_list = open(mgnt_path, 'r').readlines()
    
    del_text = '     <user id=\"%s\" type=\"pointer\"/>\n' % (agent_id)
    while del_text in text_list:
        index = del_text.index(del_text)
        del text_list[index]
    index = text_list.index('   <group name=\"%s\">\n' % (new_grp_id))
    text_list.insert(index + 2, del_text)
    open(mgnt_path, 'w').writelines(text_list)
    
def modify_param_value(sip_id, param_name, param_value):
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is', sip_id
        return None
    if param_name.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'param_name is', param_name
        return None
    if param_value.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'param_value is', param_value
        return None
    result = get_agent_by_sip(sip_id)
    agent_id = result[0][0]
    id_value = get_grp_id(sip_id)
    group_id = '0'
    if id_value[0][0].strip() == '0':
        group_id = id_value[1][0]
    else:
        group_id = id_value[0][0]
    customer_id = get_customer_id(sip_id, group_id)[0][0]
    cfg_path = db_config.get_db_param()['cfg_path']
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
    sip_path = cfg_path + 'directory/' + customer_id + '/' + agent_id + '.xml'
    text_list = open(sip_path, 'r').readlines()
    for loop in range(len(text_list)):
        if ('      <param name=\"%s\"' % param_name) in text_list[loop]:
            text_list[loop] = '<param name=\"%s\" value=\"%s\"/>\n' % (param_name, param_value)
    open(sip_path, 'w').writelines(text_list)
    
    