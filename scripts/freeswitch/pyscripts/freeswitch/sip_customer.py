# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: generate the directories and management xml files
'''

import os
import db_conn
import db_config
import customer_file
import file_info

def generate_customer():
    '''
    @todo: generate customer directories and configuration
    @return: customer id list
    '''
    global CONN
    db_conn.connect_db()
    
    # 查询出所有客户id
    seqSQLCmd = 'SELECT CONVERT(id, CHAR(10)) AS id FROM tbl_customer'
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',seqSQLCmd
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'The database connection does not exist.'
        return
    ulCount = cursor.execute(seqSQLCmd) # get count of results
    results = cursor.fetchall()     # get all record results
    if len(results) == 0:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'len(results) is', len(results)
        return
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'results is:',results
    
    seqCfgCurDir = db_config.get_db_param()['cfg_path']
    if seqCfgCurDir[-1] != '/':
        seqCfgCurDir = seqCfgCurDir + '/'
    seqCfgCurDir = seqCfgCurDir + 'directory/'
    
    for loop in range(0, ulCount):
        seqCfgPath = seqCfgCurDir + str(results[loop][0]) + '/'
        if not os.path.exists(seqCfgPath):
            os.makedirs(seqCfgPath)
        
        # 生成客户文件
        customer_file.generate_customer_file(results[loop][0])
    db_conn.CONN.close()
