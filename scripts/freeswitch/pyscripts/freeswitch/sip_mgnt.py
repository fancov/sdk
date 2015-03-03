# -*- encoding=utf-8 -*-

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 10th,2015
@todo: operation of sip account
'''
import db_conn
import file_info
import db_config
import os
import sip_maker

def add_sip(sip_id):
    '''
    @param: sip_id  sip账户id
    @todo: 添加一个sip账户
    '''
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_is is', sip_id
        return None
    
    #获取sip账户的group id
    grp_id = get_grp_id(sip_id)
    
    for item in grp_id[0]:
        if str(item).strip() == '0':
            # 删除group id为0的项
            del item
        else:
            # 将sip账户添加到座席组
            add_sip_to_group(sip_id, int(item))
            

def get_grp_id(sip_id):
    '''
    @param: sip_id sip账户id
    @todo: 获取sip账户所在的座席组id列表
    '''
    
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is:', sip_id
        return
    
    global CONN
    db_conn.connect_db()
    
    #查找所有sip账户id为sip_id的座席组id
    sql_cmd = 'SELECT CONVERT(tbl_agent.group1_id,CHAR(10)) AS group1_id,CONVERT(tbl_agent.group2_id,CHAR(10)) AS group2_id FROM tbl_agent WHERE tbl_agent.sip_id = %d' % (sip_id)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',sql_cmd
    
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'The database connection does not exist.'
        return 
    cursor.execute(sql_cmd)
    grp_id = cursor.fetchall()
    if len(grp_id) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'len(grp_id) is', len(grp_id)
        return
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'result:',grp_id
    
    #返回座席组id列表，因为一个sip账户很可能属于多个账户
    return grp_id

def get_customer_id(sip_id, grp_id):
    '''
    @param: sip_id sip账户id; grp_id sip账户所在的座席组id
    @todo: 根据sip的id获取它所属客户的id
    '''
    
    global CONN
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is:', sip_id
        return None
    if str(grp_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is:', grp_id
        return None
    
    db_conn.connect_db()
    # 获取座席组id为grp_id所属的客户id
    sql_cmd = 'SELECT CONVERT(tbl_group.customer_id,CHAR(10)) AS customer_id FROM tbl_group WHERE tbl_group.id = %s' % (grp_id)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',sql_cmd
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'The database connection does not exist.'
        return
    cursor.execute(sql_cmd)
    customer_id = cursor.fetchall()
    db_conn.CONN.close()
    if len(customer_id) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is:', customer_id
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is:', customer_id
    return customer_id

def get_agent_by_sip(sip_id):
    '''
    @param: sip_id sip账户id
    @todo: 获取sip账户对应的座席组id
    '''
    global CONN
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is',sip_id
        return None
    db_conn.connect_db()
    
    # 获取sip id是sip_id的agent id
    sql_cmd = 'SELECT CONVERT(id, CHAR(10)) AS id FROM tbl_agent WHERE tbl_agent.sip_id = %d' % (sip_id)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:', sql_cmd
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'The database connection does not exist.'
        return
    cursor.execute(sql_cmd)
    result = cursor.fetchall()
    if len(result) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'len(result) is', len(result)
        return
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'result:',result
    return result

def add_sip_to_group(sip_id, grp_id):
    '''
    @param: sip_id sip账户id; grp_id sip账户所属座席组id
    @todo: 将sip账户添加到group中
    '''
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
    
    #获取客户id
    customer_id = result[0][0]
    #获取座席组id
    agent_id = get_agent_by_sip(sip_id)[0][0]
    
    cfg_path = db_config.get_db_param()['cfg_path']
    #构造以'/'结尾的路径字符串
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'cfg_path is:',cfg_path
    
    # 客户配置文件所在路径设置:假如customer id是1，那么客户配置文件是 %cfg_path%/directory/1.xml
    customer_path = cfg_path + 'directory/' + customer_id + '.xml'
    if os.path.exists(customer_path) is False:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_path is:',customer_path
        return None
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_path is:',customer_path
    
    # sip账户配置文件路径设置：假设customer id是1，座席组id是100，那么sip账户配置文件是 %cfg_path%/directory/1/100.xml
    sip_path = cfg_path + 'directory/' + customer_id + '/' + agent_id + '.xml'
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_path is:', sip_path
    
    # 读取配置文件，按行读取并组成Python列表
    mgnt_text = open(customer_path, 'r').readlines()
    if mgnt_text == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'mgnt_text is empty.'
        return
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'mgnt_text is:', mgnt_text
    
    # 构造新的sip配置字符串并添加
    add_text = '     <user id=\"%s\" type=\"pointer\"/>\n' % (agent_id)
    if add_text in mgnt_text:
        return
    
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'add_text is:', add_text
    try:
        mgnt_index = mgnt_text.index('   <group name=\"%s-%s\">\n' % (customer_id, grp_id))
        if mgnt_index < 0:
            print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'mgnt_text does not exist.'
            return
        mgnt_text.insert(mgnt_index + 2, add_text)
        open(customer_path, 'w').writelines(mgnt_text)
        # 生成sip账户
        sip_maker.make_sip(sip_id, customer_id, sip_path)
    except Exception as e:
        print str(e)

def del_sip_from_group(agent_id, customer_id):
    '''
    @param: agent_id 座席组id; customer_id客户id
    @todo: 将一个sip账户从一个座席组中删除
    '''
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
    if os.path.exists(sip_path):
        os.remove(sip_path)
        
    mgnt_path = cfg_path + 'directory/' + str(customer_id) + '.xml'
    mgnt_text = open(mgnt_path,'r').readlines()
    if mgnt_text == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'mgnt_text is', mgnt_text
        return
    index = mgnt_text.index('     <user id=\"%d\" type=\"pointer\"/>\n' % agent_id)
    if index < 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'The index does not exist'
        return
    del mgnt_text[index]
    open(mgnt_path, 'w').writelines(mgnt_text)
   
def change_agent_group(sip_id, new_grp_id):
    '''
    @param sip_id: sip账户id
    @param new_grp_id: 新的座席组id
    @todo:将sip账户更改到另外一座席组
    '''
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is ',sip_id
        return None
    if str(new_grp_id).strip() == '':
        return file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'new_grp_id is', new_grp_id
        return None
    
    # 获取到座席组id
    result = get_agent_by_sip(sip_id)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'result is:', result
    
    #获取到customer id
    customer_id = get_customer_id(sip_id, new_grp_id)[0][0]
    agent_id = result[0][0]
    cfg_path = db_config.get_db_param()['cfg_path']
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
    mgnt_path = cfg_path + 'directory/' + str(customer_id) + '.xml'
    text_list = open(mgnt_path, 'r').readlines()
    if text_list == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'text_list is', text_list
        return
    
    del_text = '     <user id=\"%s\" type=\"pointer\"/>\n' % (agent_id)
    while True:
        if del_text in text_list:
            index = text_list.index(del_text)
            del text_list[index]
        else:
            break
    
    group_text = '   <group name=\"%s-%s\">\n' % (customer_id, new_grp_id)
    if group_text in text_list:
        index = text_list.index(group_text)
        text_list.insert(index + 2, del_text)
    else:
        index = text_list.index('   </group>\n')
        add_text = '   <group name=\"%s-%s\">\n    <users>\n     <user id="%s" type="pointer"/>\n    </users>\n   </group>\n' % (customer_id, new_grp_id, agent_id)
        text_list.insert(index + 1, add_text)
    open(mgnt_path, 'w').writelines(text_list)
    
def modify_param_value(sip_id, param_name, param_value):
    '''
    @param sip_id: sip账户id
    @param param_name: 参数名字
    @param param_value: 参数值
    @todo: 修改参数的值
    '''
    if str(sip_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'sip_id is', sip_id
        return None
    if param_name.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'param_name is', param_name
        return None
    if param_value.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'param_value is', param_value
        return None
    
    # 获取座席id
    result = get_agent_by_sip(sip_id)
    agent_id = result[0][0]
    
    # 获取座席组id
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
    if text_list == []:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'text_list is', text_list
        return
    print text_list
    for loop in range(len(text_list)):
        if ('   <param name=\"%s\"' % param_name) in text_list[loop]:
            text_list[loop] = '   <param name=\"%s\" value=\"%s\"/>\n' % (param_name, param_value)
    open(sip_path, 'w').writelines(text_list)
    
    