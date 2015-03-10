# -*- encoding=utf-8 -*-

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: generate agent group
'''
import sip_account
import db_conn
import file_info
import types

def generate_group(doc, grp_id, customer_id):
    '''
    @param doc: 文件对象
    @param grp_id: 座席组id
    @param customer_id: 客户id
    @todo: 生成座席组  
    '''
    if doc is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'doc is', doc
        return
    if grp_id == '' or grp_id is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_id is',grp_id
        return
    if customer_id == '' or customer_id is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is',customer_id
        return
    
    # 查找出所有group1_id或者group2_id为`grp_id`的座席id
    sql_cmd = 'SELECT CONVERT(id, CHAR(10)) AS id FROM tbl_agent WHERE tbl_agent.group1_id = %s OR tbl_agent.group2_id = %s' % (grp_id, grp_id)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:', sql_cmd
    
    db_conn.connect_db()
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'The database connection does not exist.'
        return
    mem_cnt = cursor.execute(sql_cmd)
    grp_rslt = cursor.fetchall() # get all results
    if len(grp_rslt) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'len(grp_rslt) is', len(grp_rslt)
        return
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'grp_rslt is', grp_rslt
    
    group_node = doc.createElement('group')
    group_node.setAttribute('name', str(customer_id) + '-' + str(grp_id))
    users_node = doc.createElement('users')
    for loop in range(0, mem_cnt):
        user_node = doc.createElement('user')
        user_node.setAttribute('id', str(grp_rslt[loop][0]) if type(grp_rslt[loop][0]) == types.IntType else grp_rslt[loop][0])
        user_node.setAttribute('type', 'pointer')
        users_node.appendChild(user_node)
        sip_account.generate_sip_account( grp_rslt[loop][0], customer_id)
    
    group_node.appendChild(users_node)
    return group_node
