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
    if customer_id is '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is', customer_id
        return None
    sql_cmd = 'select distinct convert(id, CHAR(10)) as id from tbl_group where tbl_group.customer_id = %s' % customer_id
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:', sql_cmd
    doc = Document()
    
    cfg_path = db_config.get_db_param()['cfg_path']
    if cfg_path[-1] != '/':
        cfg_path = cfg_path + '/'
    
    if os.path.exists(cfg_path) is not True:
        os.makedirs(cfg_path)
        
    cfg_path = cfg_path + 'directory/' + str(customer_id) + '.xml'
    cursor = db_conn.CONN.cursor()
    cus_count = cursor.execute(sql_cmd)
    grps_result = cursor.fetchall()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'grp_rslt:', grps_result
    (include_node ,groups_node) = customer_head.generate_customer_head(cfg_path, doc)
    for loop in range(0, cus_count):
        group_node = group.generate_group(doc, grps_result[loop][0], customer_id)
        groups_node.appendChild(group_node)
        doc.appendChild(include_node)
        dom_to_xml.dom_to_pretty_xml(cfg_path, doc)
        dom_to_xml.del_xml_head(cfg_path)
