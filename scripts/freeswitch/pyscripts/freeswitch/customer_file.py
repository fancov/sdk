# -*- encoding=utf-8 -*-
'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: generate the head of customer
'''
from xml.dom.minidom import Document
import customer_head
import group
import dom_to_xml
import db_conn
import file_info
import db_config
import os

def generate_customer_file(customer_id):
    '''
    @param customer_id: 客户id
    @todo: 生成客户配置文件
    '''
    if str(customer_id).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is', customer_id
        return None
    
    # 查找出所有customer id是`customer_id`的 group id
    sql_cmd = 'SELECT DISTINCT CONVERT(id, CHAR(10)) AS id FROM tbl_group WHERE tbl_group.customer_id = %s' % customer_id
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:', sql_cmd
    doc = Document()
    
    cfg_path = db_config.get_db_param()['cfg_path']
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
    
    if os.path.exists(cfg_path) is False:
        os.makedirs(cfg_path)
        
    mgnt_dir = cfg_path + 'directory/' + str(customer_id) + '/'
    if os.path.exists(mgnt_dir) is False:
        os.mkdir(mgnt_dir)
        
    cfg_path = cfg_path + 'directory/' + str(customer_id) + '.xml'
    db_conn.connect_db()
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'The database connection does not exist.'
        return
    cus_count = cursor.execute(sql_cmd)
    grps_result = cursor.fetchall()
    if len(grps_result) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'len(grps_result) is', len(grps_result)
        return 
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'grp_rslt:', grps_result
    
    # 生成客户配置文件头部
    (include_node ,groups_node) = customer_head.generate_customer_head(cfg_path, doc)
    for loop in range(0, cus_count):
        # 生成座席组并返回组节点DOM Node指针
        group_node = group.generate_group(doc, grps_result[loop][0], customer_id)
        # 将group添加至groups
        groups_node.appendChild(group_node)
        # 将include节点添加至doc
        doc.appendChild(include_node)
        # 生成xml
        dom_to_xml.dom_to_pretty_xml(cfg_path, doc)
        # 去掉xml文件头部的xml声明
        dom_to_xml.del_xml_head(cfg_path)
