# -*- encoding=utf-8 -*-
'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@togo: pakete the operation about database
'''

import MySQLdb
import file_info

def get_db_conn(db_host, db_username, db_password, db_name): 
    if db_host.strip() == '' is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'db_host is', db_host
        return
    if db_username.strip() == '' is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'db_username is', db_username
        return
    if db_password.strip() == '' is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'db_password is', db_password
        return
    if db_name.strip() == '' is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'db_name is', db_name
        return
    print  file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),db_host,db_username,db_password,db_name
    conn = MySQLdb.connect(db_host,  db_username, db_password, db_name)
    return conn

def get_field_value(table_name, field_name, conn):
    if table_name.strip() == '' is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'table_name is', table_name
        return
    if field_name.strip() == '' is True:
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'field_name is', field_name
        return
    sql_cmd = 'select distinct %s from %s' % (field_name, table_name)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'execute SQL:', sql_cmd
    cursor = conn.cursor()
    cursor.execute(sql_cmd)
    results = cursor.fetchall()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'executed results is:', results
    return results