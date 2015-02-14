# -*- coding:UTF-8 -*-

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
    sql_cmd = 'select CONVERT(id, CHAR(10)) as id from tbl_customer'
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'SQL:',sql_cmd
    cursor = db_conn.CONN.cursor()
    count = cursor.execute(sql_cmd) # get count of results
    results = cursor.fetchall()     # get all record results
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'results is:',results
    
    cfg_curdir = db_config.get_db_param()['cfg_path']
    if cfg_curdir[-1] != '/':
        cfg_curdir = cfg_curdir + '/'
    cfg_curdir = cfg_curdir + 'directory/'
    
    for loop in range(0, count):
        cfg_path = cfg_curdir + str(results[loop][0]) + '/'
        if not os.path.exists(cfg_path):
            os.makedirs(cfg_path)
        customer_file.generate_customer_file(results[loop][0])
    db_conn.CONN.close()
