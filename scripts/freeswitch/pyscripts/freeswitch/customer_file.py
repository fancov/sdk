# coding=utf-8
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

def generate_customer_file(ulCustomerID):
    '''
    @param customer_id: 客户id
    @todo: 生成客户配置文件
    '''
    if str(ulCustomerID).strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'customer_id is', ulCustomerID
        return None
    
    # 查找出所有customer id是`customer_id`的 group id
    seqSQLCmd = 'SELECT DISTINCT CONVERT(id, CHAR(10)) AS id FROM tbl_group WHERE tbl_group.customer_id = %s' % ulCustomerID
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:', seqSQLCmd
    doc = Document()
    
    seqCfgPath = db_config.get_db_param()['cfg_path']
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    
    if os.path.exists(seqCfgPath) is False:
        os.makedirs(seqCfgPath)
        
    seqMgntDir = seqCfgPath + 'directory/' + str(ulCustomerID) + '/'
    if os.path.exists(seqMgntDir) is False:
        os.mkdir(seqMgntDir)
        
    seqCfgPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    db_conn.connect_db()
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'The database connection does not exist.'
        return
    ulCusCount = cursor.execute(seqSQLCmd)
    arrGroupResult = cursor.fetchall()
    if len(arrGroupResult) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'len(grps_result) is', len(arrGroupResult)
        return 
    db_conn.CONN.close()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'grp_rslt:', arrGroupResult
    
    # 生成客户配置文件头部
    (domIncludeNode , domGroupsNode) = customer_head.generate_customer_head(seqSQLCmd, doc)
    for loop in range(0, ulCusCount):
        # 生成座席组并返回组节点DOM Node指针
        domGroupNode = group.generate_group(doc, arrGroupResult[loop][0], ulCustomerID)
        # 将group添加至groups
        domGroupsNode.appendChild(domGroupNode)
        # 将include节点添加至doc
        doc.appendChild(domIncludeNode)
        # 生成xml
        dom_to_xml.dom_to_pretty_xml(seqSQLCmd, doc)
        # 去掉xml文件头部的xml声明
        dom_to_xml.del_xml_head(seqSQLCmd)
