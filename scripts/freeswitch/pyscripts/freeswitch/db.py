# -*- encoding=utf-8 -*-
'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: pakete the operation about database
'''

import MySQLdb
import file_info

def get_db_conn(db_host, db_username, db_password, db_name): 
    '''
    @param db_host: 数据库主机ip
    @param db_username: 数据库用户名
    @param db_password: 数据库密码
    @param db_name: 数据库名
    @todo: 连接数据库
    '''
    if db_host.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'db_host is', db_host
        return
    if db_username.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'db_username is', db_username
        return
    if db_password.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'db_password is', db_password
        return
    if db_name.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'db_name is', db_name
        return
    print  file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),db_host,db_username,db_password,db_name
    conn = MySQLdb.connect(db_host,  db_username, db_password, db_name)
    return conn

def get_field_value(table_name, field_name, conn):
    '''
    @param table_name: 数据库表名
    @param field_name: 字段名称
    @param conn: 数据库连接
    @todo: 从数据库字段中获取sip列表
    '''
    if table_name.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'table_name is', table_name
        return
    if field_name.strip() == '':
        print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'field_name is', field_name
        return
    sql_cmd = 'SELECT DISTINCT %s FROM %s' % (field_name, table_name)
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(),'execute SQL:', sql_cmd
    cursor = conn.cursor()
    cursor.execute(sql_cmd)
    results = cursor.fetchall()
    print file_info.get_file_name(),file_info.get_line_number(),file_info.get_function_name(), 'executed results is:', results
    return results